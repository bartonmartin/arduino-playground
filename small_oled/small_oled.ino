#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>


#define PROGRAM_NAME "OLED_OTA"


// SCL GPIO5
// SDA GPIO4
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

#define PIN_BUTTON D3

// wifi network stuff
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"

// 192.168.2.xxx port 80
ESP8266WebServer server(80);
ESP8266WiFiMulti wiFiMulti;

String text = "Hello";
boolean displayReady = false;

// Button timing variables
int debounce = 20;                  // ms debounce period to prevent flickering when pressing or releasing the button
int DCgap = 150;                    // max ms between clicks for a double click event
int holdTime = 1000;                // ms hold period: how long to wait for press+hold event
int longHoldTime = 3000;            // ms long hold period: how long to wait for press+hold event

// Button variables
boolean buttonVal = HIGH;           // value read from button
boolean buttonLast = HIGH;          // buffered value of the button's previous state
boolean DCwaiting = false;          // whether we're waiting for a double click (down)
boolean DConUp = false;             // whether to register a double click on next release, or whether to wait and click
boolean singleOK = true;            // whether it's OK to do a single click
long downTime = -1;                 // time the button was pressed down
long upTime = -1;                   // time the button was released
boolean ignoreUp = false;           // whether to ignore the button release because the click+hold was triggered
boolean waitForUp = false;          // when held, whether to wait for the up event
boolean holdEventPast = false;      // whether or not the hold event happened already
boolean longHoldEventPast = false;  // whether or not the long hold event happened already


void setup()   {
  Serial.begin(115200);
  logMessage("setup start");

  pinMode(PIN_BUTTON, INPUT);

  setupDisplay();
  setupHttpServer();
  setupArduinoOta();
}


void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  handleButton();
}


void setupDisplay() {
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  // draw a single pixel
  display.drawPixel(10, 10, WHITE);
  // Show the display buffer on the hardware.
  // NOTE: You _must_ call display after making any drawing commands
  // to make them visible on the display hardware!
  display.display();
  delay(2000);
  display.clearDisplay();

  displayReady = true;

  logMessage("display ready");
  delay(1000);
}


void runTextTest() {
  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Hello, world!");
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.println(3.141592);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
  display.display();
  delay(1000);
  display.clearDisplay();
}


void setupHttpServer() {
  logMessage("HTTP server start setup");
  // wifi settings
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  // waiting for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    logAddMessage(".");
  }
  logAddMessage("Connected to ");
  logMessage(WIFI_SSID);
  logMessage(String("IP address: ") + ip2Str(WiFi.localIP()));

  // definition of all endpoints
  server.on("/", handleRoot);
  server.on("/display", handleDisplay);
  server.onNotFound(handleNotFound);

  // starting http server
  server.begin();
  logMessage("HTTP server started");
}


void setupArduinoOta() {
  logMessage("ArduinoOTA Start setup");
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(PROGRAM_NAME);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    logMessage("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    logMessage("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      logMessage("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      logMessage("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      logMessage("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      logMessage("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      logMessage("End Failed");
    }
  });
  ArduinoOTA.begin();
  logMessage("ArduinoOTA Ready");
  logMessage(String("IP address: ") + ip2Str(WiFi.localIP()));
}


void handleButton() {
  // Get button event and act accordingly
  int b = checkButton();
  if (b == 1) clickEvent();
  if (b == 2) doubleClickEvent();
  if (b == 3) holdEvent();
  if (b == 4) longHoldEvent();
}


/*

   HTTP Server functions

*/


void handleRoot() {
  logEndpointMessage(" / root");

  int httpRequestCode = 200; // OK

  String paragraph = String("<p>");
  String lineBreak = String("<br>");
  String buttons = textToButton("Root", "/");

  String endpoints = textToAhref("/switchOn") + paragraph
                     + textToAhref("/switchOff") + paragraph
                     + textToAhref("/light?number=1&isOn=true") + paragraph
                     + String("<i>Try <b>alpha feature: </b></i>") + textToAhref("/rgb?r=1&g=0&b=0") + paragraph;

  String autoRefresh = String("<meta http-equiv=\"refresh\" content=\"5\">"); //refresh every 5 seconds

  String style = String("<style>body {background-color: black;color: white;}</style>");
  String title = String("<title>ESP8266 Light server</title>");
  String head = String("<head>") + style + title + String("</head>");
  String body = String("<body>")
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("<h4>HTTP server works!</h4>") + paragraph
                + String("This is a super simple website, running on an ESP8266 based micro controller :)") + paragraph
                + String("You can see real time temperature and humidy and also you can control a relay.") + paragraph
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("Or call any <b>endpoint</b> via browser or an app:") + paragraph + lineBreak
                + endpoints + paragraph + lineBreak
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("<i>Just for debugging purposes, here is <b>status of all lights:</b>") + paragraph + lineBreak
                + String("</i></body>");
  String rootContent = String("<html>") + autoRefresh + head + body + String("</html>");

  server.send(httpRequestCode, "text/html", rootContent);
}

void handleDisplay() {
  logEndpointMessage(" / display");

  int httpRequestCode = 200; // OK
  String responseMessage = "";

  String displayText = "";

  logMessage("Arguments:");
  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));

    if (argumentName == "text") {
      displayText = argumentValue;
      logMessage("");
      String logText = String("the text ") + argumentValue;
      responseMessage =  String("it works!!! text was recognized, here it is: ") + argumentValue;
      logMessage(logText);
      delay(500);
    } else {
      logAddMessage(String(i) + " : ");  //print id
      logAddMessage("\"" + argumentName + "\" ");  //print name
      logAddMessage("\"" + argumentValue + "\" ");  //print value
      responseMessage =  String("nothing recognized");
    }
  }

  server.send(httpRequestCode, "text/plain", responseMessage);
}


void handleNotFound() {
  logEndpointMessage("404 - " + server.uri());

  int httpRequestCode = 404; // ERROR
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(httpRequestCode, "text/plain", message);
}



void sendHttpRequest(String command) {
  if ((wiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;
    String url = String("http://192.168.2.149/") + command;

    logAddMessage("[HTTP] begin...\n");
    if (http.begin(client, url)) {  // HTTP

      logAddMessage("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
          logMessage("request ok");
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        logMessage("request is fu**ed");
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
      logMessage("unable to connect");
    }
  }
}

void sendWeatherRequest() {
  if ((wiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;
    String url = String("api.openweathermap.org/data/2.5/weather?q=Prague&appid=a85e96fef27c2d27f958043f67a4034c");

    logAddMessage("[HTTP] begin...\n");
    if (http.begin(client, url)) {  // HTTP

      logAddMessage("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
          logMessage("request ok");
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        logMessage("request is fu**ed");
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
      logMessage("unable to connect");
    }
  }
}


/*

   Physical button control

*/


int checkButton() {
  int event = 0;
  buttonVal = digitalRead(PIN_BUTTON);
  // Button pressed down
  if (buttonVal == LOW && buttonLast == HIGH && (millis() - upTime) > debounce)
  {
    downTime = millis();
    ignoreUp = false;
    waitForUp = false;
    singleOK = true;
    holdEventPast = false;
    longHoldEventPast = false;
    if ((millis() - upTime) < DCgap && DConUp == false && DCwaiting == true)  DConUp = true;
    else  DConUp = false;
    DCwaiting = false;
  }
  // Button released
  else if (buttonVal == HIGH && buttonLast == LOW && (millis() - downTime) > debounce)
  {
    if (not ignoreUp)
    {
      upTime = millis();
      if (DConUp == false) DCwaiting = true;
      else
      {
        event = 2;
        DConUp = false;
        DCwaiting = false;
        singleOK = false;
      }
    }
  }
  // Test for normal click event: DCgap expired
  if ( buttonVal == HIGH && (millis() - upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true && event != 2)
  {
    event = 1;
    DCwaiting = false;
  }
  // Test for hold
  if (buttonVal == LOW && (millis() - downTime) >= holdTime) {
    // Trigger "normal" hold
    if (not holdEventPast)
    {
      event = 3;
      waitForUp = true;
      ignoreUp = true;
      DConUp = false;
      DCwaiting = false;
      //downTime = millis();
      holdEventPast = true;
    }
    // Trigger "long" hold
    if ((millis() - downTime) >= longHoldTime)
    {
      if (not longHoldEventPast)
      {
        event = 4;
        longHoldEventPast = true;
      }
    }
  }
  buttonLast = buttonVal;
  return event;
}


void clickEvent() {
  showOnDisplay(String("button click: "));
  sendHttpRequest(String("button?number=1"));
}


void doubleClickEvent() {
  addToDisplay(String("button double click: "));
  sendHttpRequest(String("button?number=2"));
}


void holdEvent() {
  showOnDisplay("button hold");
  sendHttpRequest(String("light?number=1&isOn=true"));
}


void longHoldEvent() {
  showOnDisplay("button long hold");
  sendWeatherRequest();
}


/*

   Other functions

*/


void showOnDisplay(String message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println(message);
  display.display();
}


void addToDisplay(String message) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print(message);
  display.display();
}


void logEndpointMessage(String enpointName) {
  Serial.println("---------------------------");
  logMessage(getClientIp() + " is calling " + enpointName);
}


String getClientIp() {
  return server.client().remoteIP().toString();
}


String textToAhref(String text) {
  String endpointText = ip2Str(WiFi.localIP()) + text;
  String ahref = String("<a href=\"") + text + String("\">") + endpointText + String("</a>");
  return ahref;
}


String textToButton(String text, String url) {
  String endpointText = ip2Str(WiFi.localIP()) + url;
  String ahref = String("<a href=\"") + url + String("\"><button>   ") + text  + String("   </button></a> ");
  return ahref;
}

void logMessage(String text) {
  Serial.println(text);
  if (displayReady) {
    showOnDisplay(text);
  }
}

void logAddMessage(String text) {
  Serial.print(text);
  if (displayReady) {
    addToDisplay(text);
  }
}


String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}
