/* Nokia 5100 LCD Example Code
   Graphics driver and PCD8544 interface code for SparkFun's
   84x48 Graphic LCD.
   https://www.sparkfun.com/products/10168

  by: Jim Lindblom
    adapted from code by Nathan Seidle and mish-mashed with
    code from the ColorLCDShield.
  date: October 10, 2013
  license: Officially, the MIT License. Review the included License.md file
  Unofficially, Beerware. Feel free to use, reuse, and modify this
  code as you see fit. If you find it useful, and we meet someday,
  you can buy me a beer.

  This all-inclusive sketch will show off a series of graphics
  functions, like drawing lines, circles, squares, and text. Then
  it'll go into serial monitor echo mode, where you can type
  text into the serial monitor, and it'll be displayed on the
  LCD.

  This stuff could all be put into a library, but we wanted to
  leave it all in one sketch to keep it as transparent as possible.

  Hardware: (Note most of these pins can be swapped)
    Graphic LCD Pin ---------- Arduino Pin
       1-VCC       ----------------  5V
       2-GND       ----------------  GND
       3-SCE       ----------------  7
       4-RST       ----------------  6
       5-D/C       ----------------  5
       6-DN(MOSI)  ----------------  11
       7-SCLK      ----------------  13
       8-LED       - 330 Ohm res --  9
   The SCLK, DN(MOSI), must remain where they are, but the other
   pins can be swapped. The LED pin should remain a PWM-capable
   pin. Don't forget to stick a current-limiting resistor in line
   between the LCD's LED pin and Arduino pin 9!
*/

#include <Arduino.h>
#include <SoftwareSerial.h>
//#include <SPI.h>
//#include "LCD_Functions.h"

#define BAUDRATE 9600

#define PIN_BUTTON 2
#define ESP8266_RX 10
#define ESP8266_TX 3

SoftwareSerial ESP8266(ESP8266_RX, ESP8266_TX);
//#define ESP8266 Serial

/*#define SERVER "104.27.128.214"
#define HOST "api.opendota.com"
#define PATH "/api/players/65316354/wl"*/

#define SERVER "10.0.1.34"
#define PORT "5000"
#define HOST "10.0.1.34"
#define PATH "/info/65316354"
#define TAG_START "##START"
#define TAG_END   "##END"

/*void printOnScreen(const String& data)
{
  clearDisplay(WHITE);
  setStr(data.c_str(), 0, LCD_HEIGHT-8, BLACK);
  updateDisplay();
  delay(1000);
}*/

String readData(int timeout)
{
  String data = "";
  long int time = millis() + timeout;
  while (time > millis())
  {
    while (ESP8266.available())
    {
      /*char c = ESP8266.read();
      data += c;*/
      String line = ESP8266.readStringUntil('\r');

      data += line;
    }
  }

  return data;
}

bool sendRequest()
{
  String cmd = "AT+CIPSTART=0,\"TCP\",\""; cmd += SERVER; cmd += "\","; cmd += PORT;
  Serial.print("-> ");
  Serial.println(cmd);
  ESP8266.println(cmd);
  String data = readData(2000);
  //Serial.println(data);
  //printOnScreen(data);
  delay(2000);

  cmd = "GET "; cmd += PATH; cmd += " HTTP/1.1\r\n";
  cmd +="Host: "; cmd += HOST; cmd += ":"; cmd += PORT; cmd += "\r\n";
  //cmd += "Connection: keep-alive\r\n";
  cmd += "\r\n";
  Serial.print("-> ");
  Serial.println(cmd);

  String cmd2 = "AT+CIPSEND=0,"; cmd2 += String(cmd.length());
  Serial.print("-> ");
  Serial.println(cmd2);
  ESP8266.println(cmd2);
  data = readData(2000);
  //Serial.println(data);
  //printOnScreen(data);
  delay(2000);

  ESP8266.println(cmd);
  data = readData(2000);
  //Serial.println(data);
  //printOnScreen(data);

  unsigned int infiniteLoopChecker = 0;
  const unsigned int nbLoopsMax = 100;
  int start = 0;
  unsigned int dataLength = data.length();
  bool storeResult = false;
  String result = "";
  while (start < dataLength && infiniteLoopChecker < nbLoopsMax)
  {
    // get the current line
    int indexOfNewLine = data.indexOf('\n', start);
    if (indexOfNewLine == -1)
    {
      Serial.println("End of response.");
      break;
    }

    String line = data.substring(start, indexOfNewLine);

    start = indexOfNewLine + 1;

    ++infiniteLoopChecker;

    Serial.print(infiniteLoopChecker);
    Serial.print(":\t");
    Serial.println(line);

    // store the line if necessary
    if (!storeResult && line.indexOf(TAG_START) > -1)
    {
      storeResult = true;
      continue;
    }

    if (storeResult && line.indexOf(TAG_END) > -1)
    {
      break;
    }

    if (storeResult)
    {
      result += line + "\n";
    }
  }

  Serial.println(result);

  ESP8266.println("AT+CIPCLOSE=0");
  data = readData(2000);
  Serial.println(data);
  //printOnScreen(data);
  delay(1000);
}

void setup()
{
  Serial.begin(9600);
  ESP8266.begin(BAUDRATE);
  delay(10);

  pinMode(PIN_BUTTON, INPUT);

  /*lcdBegin(); // This will setup our pins, and initialize the LCD
  updateDisplay(); // with displayMap untouched, SFE logo
  setContrast(40); // Good values range from 40-60
*/
  delay(1000);

  String data;

  // We start by connecting to a WiFi network
  //Serial.println("AT+CWJAP=\"CookieMaestro\",\"UpEcOfMe\"");
  ESP8266.println("AT");
  data = readData(2000);
  Serial.println(data);
  //printOnScreen(data);
  delay(1000);

  /*ESP8266.println("AT+RST");
  data = readData(2000);
  Serial.println(data);
  //printOnScreen(data);
  delay(3000);*/

  ESP8266.println("AT+CIFSR");
  data = readData(2000);
  Serial.println(data);
  //printOnScreen(data);
  delay(1000);

  ESP8266.println("AT+CIPMUX=1");
  data = readData(2000);
  Serial.println(data);
  //printOnScreen(data);
  delay(1000);

  sendRequest();
}

// Loop turns the display into a local serial monitor echo.
// Type to the Arduino from the serial monitor, and it'll echo
// what you type on the display. Type ~ to clear the display.
void loop()
{
  /*clearDisplay(WHITE);
  setStr("ESP8266!", 0, LCD_HEIGHT-8, BLACK);
  updateDisplay();*/

  bool buttonPressed = digitalRead(PIN_BUTTON);
  if (buttonPressed)
  {
    sendRequest();

    /*ESP8266.println("AT");
    String data = readData(1000);
    Serial.println(data);*/
    //printOnScreen(data);
  }

  delay(10);

  /*while(ESP8266.available() > 0)
  {
    char a = ESP8266.read();
    if(a == '\0')
      continue;
    if(a != '\r' && a != '\n' && (a < 32))
      continue;
    Serial.print(a);
  }

  while(Serial.available() > 0)
  {
    char a = Serial.read();
    Serial.write(a);
    ESP8266.write(a);
  }*/

}
