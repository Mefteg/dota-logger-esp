/*
*  Simple HTTP get webclient test
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

#include "LCD_Functions.h"


#define BAUDRATE 9600
#define BREAK_CODE '\r'

#define INFO_UPDATE_FREQUENCY 10000 // Send a request every 10s


const char* host = "dota-logger-server.herokuapp.com";
const String EEPROMSeparator = "$-$";

String ssid     = "TEST";
String password = "TEST";
String userId   = "TEST";

// we want to launch the first request immediately
unsigned int timeOfTheLastRequest = millis() + INFO_UPDATE_FREQUENCY;

String ReadFromEEPROM(
    unsigned int addrStart, unsigned int length, char breakCode
)
{
    String data = "";

    for (unsigned int i=addrStart; i<addrStart + length; ++i)
    {
        char value = EEPROM.read(i);

        if (value == breakCode)
        {
            break;
        }

        data += value;
    }

    return data;
}

void WriteToEEPROM(
    unsigned int addrStart, unsigned int length, char breakCode,
    const String& value
)
{
    if (value.length() > length - 1)
    {
        Serial.println("Can't write on EEPROM: value length is too big.");
    }

    unsigned int i = addrStart;
    while (i<addrStart + value.length())
    {
        EEPROM.write(i, value.charAt(i));

        ++i;
    }

    EEPROM.write(i, breakCode);

    EEPROM.commit();
}

void ConnectToWiFi()
{
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 10)
    {
        delay(1000);
        Serial.print(".");

        ++i;
    }

    if (i >= 10)
    {
        Serial.println("Unable to connect to WiFi. Try using commands 'ssid:' and 'password:' to set proper values.");
        return;
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

String GetInfo()
{
    String response;

    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort))
    {
        Serial.println("connection failed");
        return response;
    }

    // We now create a URI for the request
    String url = "/info/" + userId;
    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "Connection: close\r\n\r\n");
    delay(500);


    // Read all the lines of the reply from server and print them to Serial
    while(client.available())
    {
        String line = client.readStringUntil('\r');
        response += line;
    }

    Serial.println();
    Serial.println("closing connection");

    return response;
}

void PrintOnScreen(int x, int y, const String& data)
{
    clearDisplay(WHITE);
    setStr(data.c_str(), x, y, BLACK);
    updateDisplay();
}

void DisplayInfo(const String& info)
{
    unsigned int cptLines = 0;
    const unsigned int nbLoopsMax = 100;
    const unsigned int length = info.length();
    int start = 0;
    while (start < length && cptLines < nbLoopsMax)
    {
        // get the current line
        int indexOfNewLine = info.indexOf('\n', start);
        if (indexOfNewLine == -1)
        {
            indexOfNewLine = length;
        }

        String line = info.substring(start, indexOfNewLine);

        if (line.length() > 0)
        {
            PrintOnScreen(cptLines * 8, 0, line);
        }

        start = indexOfNewLine + 1;
        ++cptLines;
    }
}

void ReadConfigFromEEPROM()
{
    String data = ReadFromEEPROM(0, 512, BREAK_CODE);
    unsigned int separatorLength = EEPROMSeparator.length();
    unsigned int indexOfFirstSeparator = data.indexOf(EEPROMSeparator);
    unsigned int indexOfSecondSeparator = data.indexOf(EEPROMSeparator, indexOfFirstSeparator + separatorLength);
    Serial.println(indexOfFirstSeparator);
    Serial.println(indexOfSecondSeparator);
    if (indexOfFirstSeparator != -1 && indexOfSecondSeparator != -1)
    {
        ssid        = data.substring(0, indexOfFirstSeparator);
        password    = data.substring(indexOfFirstSeparator + separatorLength, indexOfSecondSeparator);
        userId      = data.substring(indexOfSecondSeparator + separatorLength);
    }
}

void SaveConfigToEEPROM()
{
    String data = ssid + EEPROMSeparator + password + EEPROMSeparator + userId;
    WriteToEEPROM(0, 512, BREAK_CODE, data);
}

void setup()
{
    Serial.begin(BAUDRATE);
    delay(100);

    // we start by getting stored wifi ssid, password and user id

    Serial.println("Getting stored config...");

    EEPROM.begin(512);

    ReadConfigFromEEPROM();

    Serial.println("Done.");

    // then by connecting to a WiFi network

    Serial.println("Connecting to WiFi...");

    ConnectToWiFi();

    Serial.println("Done.");
}

void loop()
{
    // read input
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\r');
        Serial.println(String("Processing command: ") + cmd);

        if (cmd.indexOf("help") != -1)
        {
            Serial.println("Commands:\nhelp\nssid:\npassword:\nuserid:\nreset");
        }
        if (cmd.indexOf("ssid:") != -1)
        {
            ssid = cmd.substring(String("ssid:").length());
            Serial.println(String("New ssid: ") + ssid);
            SaveConfigToEEPROM();
        }
        if (cmd.indexOf("password:") != -1)
        {
            password = cmd.substring(String("password:").length());
            Serial.println(String("New password: ") + password);
            SaveConfigToEEPROM();
        }
        if (cmd.indexOf("userid:") != -1)
        {
            userId = cmd.substring(String("userid:").length());
            Serial.println(String("New userid: ") + userId);
            SaveConfigToEEPROM();
        }
        if (cmd.indexOf("reset") != -1)
        {
            Serial.println("Reset");
            ESP.restart();
        }
    }

    // get user info if the time has come and device is connected
    if (millis() > timeOfTheLastRequest + INFO_UPDATE_FREQUENCY && WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Getting DotA info...");

        String info;
        info = GetInfo();
        if (info.length() > 0)
        {
            Serial.print(info);
            //DisplayInfo(info);

            Serial.println("Done.");
        }
        else
        {
            Serial.println("An error occured while retrieving DotA info.");
        }

        timeOfTheLastRequest = millis();
    }

    delay(100);
}
