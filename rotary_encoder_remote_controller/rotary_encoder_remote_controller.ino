// Used hardware:
// D1 Mini Lite
// D1 Mini Battery shield
// HW-040 Rotary Encoder
// 0.91" OLED display

#include <Wire.h>
#include <Adafruit_SSD1306.h>


#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET      -1    // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH    128   // OLED display width, in pixels
#define SCREEN_HEIGHT   32    // OLED display height, in pixels
#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Declaration for rotary encoder
#define SW     D5             // switch
#define CLK    D6             // clock
#define DT     D7             // data


// Declaration for wifi client
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"
String serverName = "http://192.168.2.95:80";
WiFiClient client;
HTTPClient http;


// time related sutff
unsigned long lastMeasureTime = 0;
int dataReadInterval = 5;


// RGBW channgels and index pointing at the channel
int channelIndex = 0;
int channelCount = 4;
int channelValueList[4] = {0, 0, 0, 0};
int previousValues[4] = {0, 0, 0, 0};
String channelNameList[4] = {"R", "G", "B", "W"};


// program data
int currentCLK;
int previousCLK;
String encoderDirection = "";


void setup() {
  // setup serial console
  Serial.begin(115200);


  // setup wifi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  testWifiClient();


  Serial.println("setup pins");
  // setup pin mode

  pinMode(SW, INPUT_PULLUP);
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  Serial.println("begin display");
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  Serial.println("clear display");
  display.clearDisplay();

  // Draw a single pixel in white
  Serial.println("draw test pixel");
  display.drawPixel(10, 10, SSD1306_WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);

  Serial.println("clear test pixel");
  display.clearDisplay();
  previousCLK = digitalRead(CLK);
  Serial.println("read clk");
  showText(millis());
}

void loop() {
  unsigned long currentTime = millis();
  int switchValue = digitalRead(SW);


  // process data from pins
  // dataReadInterval is basically a simple debounce
  if ((currentTime - lastMeasureTime) >= dataReadInterval) {
    int dataValue = digitalRead(DT);
    currentCLK = digitalRead(CLK);

    if (currentCLK != previousCLK) {
      if (dataValue != currentCLK) {

        int stepSize = 5;
        if (channelIndex == 3) {
          stepSize = 25;
        } else {
          stepSize = 5;
        }

        int value = channelValueList[channelIndex];

        if (value <= stepSize) {
          channelValueList[channelIndex] += (stepSize / 5);
        } else {
          channelValueList[channelIndex] += stepSize;
        }


        if (channelValueList[channelIndex] >= 255) {
          channelValueList[channelIndex] = 255;
        }
        encoderDirection = "clockwise";
      } else {

        int stepSize = 5;
        if (channelIndex == 3) {
          stepSize = 25;
        } else {
          stepSize = 5;
        }

        int value = channelValueList[channelIndex];

        if (value <= stepSize) {
          channelValueList[channelIndex] -= (stepSize / 5);
        } else {
          channelValueList[channelIndex] -= stepSize;
        }


        if (channelValueList[channelIndex] <= 0) {
          channelValueList[channelIndex] = 0;
        }
        encoderDirection = "counter-clockwise";
      }
      Serial.print("encoderDirection ");
      Serial.println(encoderDirection);
      showText(currentTime);
    }

    previousCLK = currentCLK;
    lastMeasureTime = currentTime;
  }

  if (switchValue == LOW) {
    Serial.println("switch pressed");
    switchClick();
    delay(150);
  }
}


void showText(unsigned long currentTime) {
  displayData();

  Serial.print(" channelValueList[channelIndex] value: ");
  Serial.print(channelValueList[channelIndex]);
  Serial.println();

    sendApiCall();
}


void switchClick() {
  channelIndex++;
  if (channelIndex >= channelCount) {
    channelIndex = 0;
  }

  displayData();

  Serial.println("switchClick");
}

void displayData() {
  display.setTextColor(WHITE);
  display.clearDisplay();
  displayChannelName();
  displayChannelState();
  display.display();
}

void displayChannelName() {
  String channelName = channelNameList[channelIndex];
}

void displayChannelState() {
  display.setCursor(0, 0);

  displayChannel(0);
  displayChannel(1);
  displayChannel(2);

  display.setCursor(0, 18);
  displayChannel(3);
}

void displayChannel(int index) {
  String channelName = channelNameList[index];

  if (channelIndex == index) {
    display.setTextSize(2);
  } else {
    display.setTextSize(1);
  }
  display.print(channelName);
  display.print(channelValueList[index]);
  display.setTextSize(1);
  display.print(" ");
  display.setTextSize(1);
}

void sendApiCall() {
  boolean arraysNotIdentical = memcmp(previousValues, channelValueList, sizeof(previousValues)) != 0;

  if (arraysNotIdentical) {
    Serial.println("sendApiCall");

    if (WiFi.status() == WL_CONNECTED) {
      String serverPath = serverName + "/rgbSimple?r=" + channelValueList[0] + "&g=" + channelValueList[1] + "&b=" + channelValueList[2] + "&w=" + channelValueList[3] ;
      Serial.println("serverPath: " + serverPath);

      // Your Domain name with URL path or IP address with path
      http.begin(client, serverPath.c_str());

      // Send HTTP GET request
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  } else {
    Serial.println("values are identical");
  }

  memcpy(previousValues, channelValueList, sizeof channelValueList);
}

void testWifiClient() {
  Serial.println("testWifiClient");

  if (WiFi.status() == WL_CONNECTED) {
    String serverPath = serverName + "/random";

    // Your Domain name with URL path or IP address with path
    http.begin(client, serverPath.c_str());

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}
