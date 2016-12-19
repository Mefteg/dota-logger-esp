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

#define DEBUG
#define USE_SCREEN

#include <Arduino.h>
#ifdef USE_SCREEN
#include <SPI.h>
#include "LCD_Functions.h"
#else
#include <SoftwareSerial.h>
#endif //USE_SCREEN

#define BAUDRATE 9600

#define PIN_BUTTON 2
#define ESP8266_RX 10
#define ESP8266_TX 3

#ifdef DEBUG

#define SERVER "10.0.1.34"
#define PORT "5000"
#define HOST "10.0.1.34"
#define PATH "/info/65316354"

#else

#define SERVER "dota-logger-server.herokuapp.com"
#define PORT "80"
#define HOST "dota-logger-server.herokuapp.com"
#define PATH "/info/65316354"

#endif //DEBUG

#define TAG_START "##START"
#define TAG_END   "##END"

#ifdef USE_SCREEN
#define ESP8266 Serial
#else
SoftwareSerial ESP8266(ESP8266_RX, ESP8266_TX);
#endif //USE_SCREEN
String infoToDisplay;
bool refreshDisplay;

#ifdef USE_SCREEN
void printOnScreen(const String& data)
{
  clearDisplay(WHITE);
  setStr(data.c_str(), 0, 0, BLACK);
  updateDisplay();
}
#endif //USE_SCREEN

void print(const String& data)
{
#ifdef USE_SCREEN
  printOnScreen(data);
#else
  Serial.println(data);
#endif //USE_SCREEN
}

String readData(int timeout)
{
  String data = "";
  long int time = millis() + timeout;
  while (time > millis())
  {
    while (ESP8266.available())
    {
      String line = ESP8266.readStringUntil("\n");

      data += line;
    }
  }

  return data;
}

String GetInfo()
{
  String cmd = "AT+CIPSTART=0,\"TCP\",\""; cmd += SERVER; cmd += "\","; cmd += PORT;
  print(String("-> ") + cmd);
  ESP8266.println(cmd);
  String data = readData(2000);
  print(data);
  delay(2000);

  cmd = "GET "; cmd += PATH; cmd += " HTTP/1.1\r\n";
  cmd +="Host: "; cmd += HOST; cmd += ":"; cmd += PORT; cmd += "\r\n";
  cmd += "\r\n";
  print(String("-> ") + cmd);

  String cmd2 = "AT+CIPSEND=0,"; cmd2 += String(cmd.length());
  print(String("-> ") + cmd2);
  ESP8266.println(cmd2);
  data = readData(2000);
  print(data);
  delay(2000);

  // send request and get result
  print(cmd);
  ESP8266.println(cmd);
  data = readData(2000);
  print(data);
  delay(2000);

  // parse result to extract DotA info
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

  ESP8266.println("AT+CIPCLOSE=0");
  data = readData(1000);
  delay(100);

  print(String("Length: ") + String(data.length()) + String("\nLength: ") + String(result.length()));

  delay(5000);

  print(result);

  delay(5000);

  return result;
}

void setup()
{
  ESP8266.begin(BAUDRATE);
  delay(10);

  pinMode(PIN_BUTTON, INPUT);

#ifdef USE_SCREEN
  lcdBegin(); // This will setup our pins, and initialize the LCD
  updateDisplay(); // with displayMap untouched, SFE logo
  setContrast(40); // Good values range from 40-60
  delay(10);
#else
  Serial.begin(BAUDRATE);
#endif //USE_SCREEN

  infoToDisplay = "INFO";

  String data;

  // We start by connecting to a WiFi network
  //Serial.println("AT+CWJAP=\"CookieMaestro\",\"UpEcOfMe\"");
  ESP8266.println("AT");
  print("-> AT");
  data = readData(2000);
  print(data);
  delay(1000);

  ESP8266.println("AT+CIFSR");
  print("-> AT+CIFSR");
  data = readData(2000);
  print(data);
  delay(1000);

  ESP8266.println("AT+CIPMUX=1");
  print("-> AT+CIPMUX=1");
  data = readData(2000);
  print(data);
  delay(1000);

  infoToDisplay = GetInfo();

  refreshDisplay = true;
}

// Loop turns the display into a local serial monitor echo.
// Type to the Arduino from the serial monitor, and it'll echo
// what you type on the display. Type ~ to clear the display.
void loop()
{
  if (refreshDisplay)
  {
#ifdef USE_SCREEN
    clearDisplay(WHITE);
    //setStr("ESP8266!", 0, LCD_HEIGHT-8, BLACK);
    updateDisplay();
#endif //USE_SCREEN

    print(String("info:\n") + infoToDisplay);

    refreshDisplay = false;
  }

  bool buttonPressed = digitalRead(PIN_BUTTON);
  if (buttonPressed)
  {
    infoToDisplay = GetInfo();
    refreshDisplay = true;
  }

  delay(10);
}
