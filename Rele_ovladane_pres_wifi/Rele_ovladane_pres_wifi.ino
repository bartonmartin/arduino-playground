// HTTP server running on an ESP8266

// include all libraries used in the project
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WEMOS_SHT3X.h>


#define PROGRAM_NAME "3_RELAYS_BUTTON_OTA"


// definition of used pins
#define PIN_RELAY_1         D8
#define PIN_RELAY_2         D7
#define PIN_RELAY_3         D6
#define PIN_BUTTON          D3


// definition of ID for each relay
#define RELAY_COUNT          3


// wifi network stuff
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"


// 192.168.2.xxx port 80
ESP8266WebServer server(80);
ESP8266WiFiMulti wiFiMulti;


// I2C Interface digital temperature and humidity sensor
SHT3X sht30(0x45);


// relays
int relayIndex = 0;                                                   // index is used to select correct ID and PIN from arrays
boolean relayOnArray[RELAY_COUNT] = {false, false, false};                       // used to store relays state ON/OFF
int relayPinArray[RELAY_COUNT] = {PIN_RELAY_1, PIN_RELAY_2, PIN_RELAY_3};      // used to store relays PIN


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




/*

   Main program

*/


void setup() {
  // serial monitor setup
  Serial.begin(115200);

  // pins setup
  for (int i = 0; i < RELAY_COUNT; i++) {
    pinMode(relayPinArray[i], OUTPUT);
  }
  pinMode(PIN_BUTTON, INPUT);

  // setting all rellays to default state == OFF
  switchAllRelays();
  setupHttpServer();
  setupArduinoOta();
}


void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  // Get button event and act accordingly
  int b = checkButton();
  if (b == 1) clickEvent();
  if (b == 2) doubleClickEvent();
  if (b == 3) holdEvent();
  if (b == 4) longHoldEvent();
}


void setupArduinoOta() {
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
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void setupHttpServer() {
  // wifi settings
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");

  // waiting for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // definition of all endpoints
  server.on("/", handleRoot);
  server.on("/switchOff", handleTurnOff);
  server.on("/switchOn", handleTurnOn);
  server.on("/light", handleRelay);
  server.on("/button", handleButtonWeb);
  server.on("/rgb", handleRgb);
  server.onNotFound(handleNotFound);

  // starting http server
  server.begin();
  Serial.println("HTTP server started");
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
  if (relayIndex >= RELAY_COUNT) {
    relayIndex = 0;
  }
  logEndpointMessage((String("button click: ") + relayIndex));
  switchRelay();
}


void doubleClickEvent() {
  relayIndex++;
  if (relayIndex >= RELAY_COUNT) {
    relayIndex = 0;
  }
  logEndpointMessage((String("button double click: ") + relayIndex));
}


void holdEvent() {
  logEndpointMessage("button hold");
}


void longHoldEvent() {
  logEndpointMessage("button long hold");
}


/*

   HTTP Server functions

*/


void handleRoot() {
  Serial.println("");
  logEndpointMessage(" / root");

  int httpRequestCode = 200; // OK

  String paragraph = String("<p>");
  String lineBreak = String("<br>");
  String buttons = textToButton("Root", "/");

  for (int i = 0; i < RELAY_COUNT; i++) {
    buttons += textToButton(i);
  }

  if (sht30.get() == 0) {
    logEndpointMessage("Reading temp and humidity");
    Serial.println("Get temp!");
  }
  else
  {
    Serial.println("Error sht30 value!");
  }

  String temperature = String("Temperature in Celsius : ") + sht30.cTemp + String(" Celsius (+- 1 Celsius)");
  String pressure = String("Relative humidity : ") + sht30.humidity + String(" % (+- 5%)");

  String endpoints = textToAhref("/switchOn") + paragraph
                     + textToAhref("/switchOff") + paragraph
                     + textToAhref("/light?number=1&isOn=true") + paragraph
                     + String("<i>Try <b>alpha feature: </b></i>") + textToAhref("/rgb?r=1&g=0&b=0") + paragraph;

  String relaysState = relayState(1, boolToString(!relayOnArray[0])) + paragraph
                       + relayState(2, boolToString(relayOnArray[1])) + paragraph
                       + relayState(3, boolToString(relayOnArray[2])) + paragraph;

  String autoRefresh = String("<meta http-equiv=\"refresh\" content=\"5\">"); //refresh every 5 seconds

  String style = String("<style>body {background-color: black;color: white;}</style>");
  String title = String("<title>ESP8266 Light server</title>");
  String head = String("<head>") + style + title + String("</head>");
  String body = String("<body>")
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("<h4>Tap button to switch light</h4>") + lineBreak
                + buttons + lineBreak
                + paragraph + lineBreak
                + temperature + lineBreak
                + pressure + lineBreak
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
                + relaysState + paragraph + lineBreak
                + String("</i></body>");
  String rootContent = String("<html>") + autoRefresh + head + body + String("</html>");

  server.send(httpRequestCode, "text/html", rootContent);
}


void sendHttpRequest(int r, int g, int b) {
  if ((wiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;
    String url = String("http://192.168.2.243/rgb?r=") + r + String("&g=") + g + String("&b=") + b;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, url)) {  // HTTP

      Serial.print("[HTTP] GET...\n");
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
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
}


void handleRgb() {
  Serial.println("");
  logEndpointMessage(" / rgb");

  int httpRequestCode = 200; // OK

  int r = 0;
  int g = 0;
  int b = 0;
  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    Serial.print(String(i) + " ");  //print id
    Serial.print("\"" + argumentName + "\" ");  //print name
    Serial.println("\"" + argumentValue + "\"");  //print value

    if (argumentName == "r") {
      r = server.arg(i).toInt();
    }

    if (argumentName == "g") {
      g = server.arg(i).toInt();
    }

    if (argumentName == "b") {
      b = server.arg(i).toInt();
    }
  }

  sendHttpRequest(r, g, b);

  String paragraph = String("<p>");
  String lineBreak = String("<br>");

  String style = String("<style>body {background-color: black;color: white;}</style>");
  String title = String("<title>ESP8266 Light server</title>");
  String head = String("<head>") + style + title + String("</head>");
  String body = String("<body>")
                + String("<h1>HTTP client also works!</h1>") + paragraph
                + lineBreak
                + String("You have just activated LED RGB strip connected to <b>ANOTHER Arduino</b> over the local network :)") + paragraph
                + lineBreak
                + String("</body>");
  String rgbContent = String("<html>") + head + body + String("</html>");

  server.send(httpRequestCode, "text/html", rgbContent);
}


void handleTurnOn() {
  logEndpointMessage(" / switchOn");

  int httpRequestCode = 200; // OK
  relayOnArray[0] = HIGH;
  relayOnArray[1] = HIGH;
  relayOnArray[2] = HIGH;
  switchAllRelays();
  //server.send(httpRequestCode, "text / plain", "prave si aktivoval vsechny rele pres wifi pomoci prohlizece !!!!");
  handleRoot();
}


void handleTurnOff() {
  logEndpointMessage(" / switchOff");

  int httpRequestCode = 200; // OK
  relayOnArray[0] = LOW;
  relayOnArray[1] = LOW;
  relayOnArray[2] = LOW;
  switchAllRelays();
  //server.send(httpRequestCode, "text / plain", "prave si deaktivoval vsechny rele pres wifi pomoci prohlizece");
  handleRoot();
}


void handleRelay() {
  logEndpointMessage(" / light");

  int httpRequestCode = 200; // OK
  int relayNumber = 0;

  boolean isRelayOn = false;

  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    Serial.print(String(i) + " ");  //print id
    Serial.print("\"" + argumentName + "\" ");  //print name
    Serial.println("\"" + argumentValue + "\"");  //print value

    if (argumentName == "number") {
      relayNumber = server.arg(i).toInt();
    }
    if (argumentName == "isOn") {
      isRelayOn = server.arg(i) == "true";
    }
  }

  switch (relayNumber) {
    case 1:
      relayOnArray[0] = isRelayOn;
      digitalWrite(relayPinArray[0], relayOnArray[0]);
      break;
    case 2:
      relayOnArray[1] = isRelayOn;
      digitalWrite(relayPinArray[1], relayOnArray[1]);
      break;
    case 3:
      relayOnArray[2] = isRelayOn;
      digitalWrite(relayPinArray[2], relayOnArray[2]);
      break;
  }

  String status = isRelayOn ? " is ON" : " is OFF";
  String message =  "light number: " + String(relayNumber) + status;
  //server.send(httpRequestCode, "text/plain", message);
  handleRoot();
}


void handleButtonWeb() {
  logEndpointMessage(" / button");

  int httpRequestCode = 200; // OK

  int buttonNumber = -1;

  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    Serial.print(String(i) + " ");  //print id
    Serial.print("\"" + argumentName + "\" ");  //print name
    Serial.println("\"" + argumentValue + "\"");  //print value

    if (argumentName == "number") {
      buttonNumber = server.arg(i).toInt();
    }
  }

  if (buttonNumber == -1) {
    server.send(httpRequestCode, "text/plain", "fail");
  } else {
    if (buttonNumber == 1) clickEvent();
    if (buttonNumber == 2) doubleClickEvent();
    if (buttonNumber == 3) holdEvent();
    if (buttonNumber == 4) longHoldEvent();
    server.send(httpRequestCode, "text/plain", "OK");
  }
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


/*

   Other functions

*/


void switchRelay() {
  boolean relayOn = relayOnArray[relayIndex];
  int relayId = relayIndex + 1;
  int relayPin = relayPinArray[relayIndex];

  if (relayOn) {
    relayOn = false;
    Serial.println(String("switch OFF light ") + relayId);
  } else {
    relayOn = true;
    Serial.println(String("switch ON light ") + relayId);
  }

  // write new relay value
  relayOnArray[relayIndex] = relayOn;
  digitalWrite(relayPin , relayOn ? HIGH : LOW);
}


String relayState(int relayId, String state) {
  return String("Light ") + relayId + String(" : ") + state;
}


String boolToString(bool b) {
  return b ? "ON" : "OFF";
}


String boolToStringValue(bool b) {
  return b ? "true" : "false";
}


void switchAllRelays() {
  digitalWrite(PIN_RELAY_1, relayOnArray[0]);
  digitalWrite(PIN_RELAY_2, relayOnArray[1]);
  digitalWrite(PIN_RELAY_3, relayOnArray[2]);
}


void logEndpointMessage(String enpointName) {
  Serial.println("---------------------------");
  Serial.println(getClientIp() + " is calling " + enpointName);
}


String getClientIp() {
  return server.client().remoteIP().toString();
}


String textToAhref(String text) {
  String endpointText = ip2Str(WiFi.localIP()) + text;
  String ahref = String("<a href=\"") + text + String("\">") + endpointText + String("</a>");
  return ahref;
}


String textToButton(int relayArrayIndex) {
  String text = "";
  String url = "";

  boolean isRelayOn = relayOnArray[relayArrayIndex];
  int relayId = relayArrayIndex + 1;

  if (relayArrayIndex == 0) {
    url = String("/light?number=") + relayId + String("&isOn=") + boolToStringValue(!isRelayOn);
    text = String("Light ") + relayId + String(" <br>Switch ") + boolToString(isRelayOn);
  } else {
    url = String("/light?number=") + relayId + String("&isOn=") + boolToStringValue(!isRelayOn);
    text = String("Light ") + relayId + String(" <br>Switch ") + boolToString(!isRelayOn);
  }

  return textToButton(text, url);
}


String textToButton(String text, String url) {
  String endpointText = ip2Str(WiFi.localIP()) + url;
  String ahref = String("<a href=\"") + url + String("\"><button>   ") + text  + String("   </button></a> ");
  return ahref;
}


String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}
