#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_1             D1
#define PIN_2             D2
#define PIN_3             D3
#define PIN_4             D4
#define PIN_5             D5
#define PIN_6             D6
#define PIN_7             D7
#define PIN_8             D8
#define PIN_LIGHT         32

#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"
#define HTTP_REQUEST_OK     200
#define HTTP_REQUEST_PNF    404

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

// SCL GPIO5
// SDA GPIO4
#define I2C_SDA 27
#define I2C_SCL 25
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET 0  // GPIO0

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

const char HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
    <head>
        <title>ESP Pushbutton Web Server</title>
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <style>
            body {
                font-family: Arial;
                text-align: center;
                margin: 0px auto;
                padding-top: 30px;
                background-color: black;
                color: white;
            }
            .button {
                padding: 10px 20px;
                font-size: 24px;
                text-align: center;
                outline: none;
                margin: 1px;
                color: #fff;
                background-color: #2f4468;
                border: none;
                border-radius: 5px;
                box-shadow: 0 6px #999;
                cursor: pointer;
                -webkit-touch-callout: none;
                -webkit-user-select: none;
                -khtml-user-select: none;
                -moz-user-select: none;
                -ms-user-select: none;
                user-select: none;
                -webkit-tap-highlight-color: rgba(0, 0, 0, 0);
            }
            .button:hover {
                background-color: #1f2e45;
            }
            .button:active {
                background-color: #1f2e45;
                box-shadow: 0 4px #666;
                transform: translateY(2px);
            }
            .stream {
                -webkit-user-select: none;
                margin: auto;
            }
        </style>
    </head>
    <body>
        <h1>Arduino RC Car MK2</h1>
        <h3>
            ESP32 + HTML + CSS + Javascripct + 4 Motors
        </h3>
        <img class="stream" src="http:/\/192.168.2.186:81/stream" />
        <p>
            <button class="button" onmousedown="forward();" ontouchstart="forward();">
                Forward
            </button>
            <button class="button" onmousedown="backward();" ontouchstart="backward();">
                Backward
            </button>
        </p>
        <p>
            <button class="button" onmousedown="left();" ontouchstart="left();">
                Left
            </button>
            <button class="button" onmousedown="right();" ontouchstart="right();">
                Right
            </button>
        </p>
        <p>
            <button class="button" onmousedown="light();" ontouchstart="light();">
                Light
            </button>
        </p>

        <script>
            function forward() {
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/forward");
                xhr.send();
            }
            function backward() {
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/backward");
                xhr.send();
            }
            function left() {
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/left");
                xhr.send();
            }
            function right() {
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/right");
                xhr.send();
            }
            function light() {
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/light");
                xhr.send();
            }
        </script>
    </body>
</html>)rawliteral";


TwoWire DISPLAY_I2C = TwoWire(0);
Adafruit_SSD1306 display;
boolean displayReady = false;

//192.168.2.xxx port 80
WebServer server(80);

int stepLenghtLeft = 100;
int stepLenghtRight = 100;
int stepLenghtForward = 500;
int stepLenghtBackward = 500;

float speed = 100;
boolean lightOn = false;
boolean showRootLog = true;

// setting PWM properties
const int freq = 100;
const int ledChannel1 = 0;
const int ledChannel2 = 1;
const int ledChannel3 = 2;
const int ledChannel4 = 3;
const int ledChannel5 = 4;
const int ledChannel6 = 5;
const int ledChannel7 = 6;
const int ledChannel8 = 7;
const int resolution = 8;


void setup(void) {
  // setup serial console
  Serial.begin(115200);

  // value LOW == motor is on
  // value HIGH == motor is off
  // PIN_1 LOW - front right forward
  // PIN_2 LOW - front right backward
  // PIN_3 LOW - front left backward
  // PIN_4 LOW - front left forward
  // PIN_5 LOW - rear right forward
  // PIN_6 LOW - rear right backward
  // PIN_7 LOW - rear left backward
  // PIN_8 LOW - rear left forward

  // setup pin mode
  pinMode(PIN_1, OUTPUT);
  pinMode(PIN_2, OUTPUT);
  pinMode(PIN_3, OUTPUT);
  pinMode(PIN_4, OUTPUT);
  pinMode(PIN_5, OUTPUT);
  pinMode(PIN_6, OUTPUT);
  pinMode(PIN_7, OUTPUT);
  pinMode(PIN_8, OUTPUT);
  pinMode(PIN_LIGHT, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  ledcSetup(ledChannel1, freq, resolution);
  ledcSetup(ledChannel2, freq, resolution);
  ledcSetup(ledChannel3, freq, resolution);
  ledcSetup(ledChannel4, freq, resolution);
  ledcSetup(ledChannel5, freq, resolution);
  ledcSetup(ledChannel6, freq, resolution);
  ledcSetup(ledChannel7, freq, resolution);
  ledcSetup(ledChannel8, freq, resolution);
  ledcAttachPin(PIN_1, ledChannel1);
  ledcAttachPin(PIN_2, ledChannel2);
  ledcAttachPin(PIN_3, ledChannel3);
  ledcAttachPin(PIN_4, ledChannel4);
  ledcAttachPin(PIN_5, ledChannel5);
  ledcAttachPin(PIN_6, ledChannel6);
  ledcAttachPin(PIN_7, ledChannel7);
  ledcAttachPin(PIN_8, ledChannel8);

  digitalWrite(PIN_1, HIGH);
  digitalWrite(PIN_2, HIGH);
  digitalWrite(PIN_3, HIGH);
  digitalWrite(PIN_4, HIGH);
  digitalWrite(PIN_5, HIGH);
  digitalWrite(PIN_6, HIGH);
  digitalWrite(PIN_7, HIGH);
  digitalWrite(PIN_8, HIGH);
  digitalWrite(LED_BUILTIN, LOW);
  switchLight();

  setupDisplay();
  setupHttpServer();
}


void loop(void) {
  server.handleClient();
}


void setupDisplay() {
  DISPLAY_I2C.begin(I2C_SDA, I2C_SCL);
  display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &DISPLAY_I2C, OLED_RESET);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // init done

  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();

  displayReady = true;

  logMessage("display ready");
  delay(1000);
}


void setupHttpServer() {
  // set wifi mode
  WiFi.mode(WIFI_STA);
  // join wifi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  logAddMessageLine("wifi begin");

  // waiting for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    logAddMessage(".");
  }

  logMessage("Connected to: " + String(WIFI_SSID));
  logAddMessageLine("IP address: ");
  logAddMessageLine(ip2Str(WiFi.localIP()));

  // define all available http endpoints
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/right", handleRight);
  server.on("/left", handleLeft);
  server.on("/light", handleLight);
  server.on("/stepLenght", handleSwitchOffDelay);
  server.onNotFound(handleNotFound);

  // start server
  server.begin();
  logAddMessageLine("HTTP server started");
}

void handleForward() {
  logEndpointMessage("/ forward");
  server.send(HTTP_REQUEST_OK);

  ledcWrite(ledChannel2, speed);
  ledcWrite(ledChannel3, speed);
  ledcWrite(ledChannel6, speed);
  ledcWrite(ledChannel7, speed);
  delay(stepLenghtBackward);
  ledcWrite(ledChannel2, 0);
  ledcWrite(ledChannel3, 0);
  ledcWrite(ledChannel6, 0);
  ledcWrite(ledChannel7, 0);
}

void handleBackward() {
  logEndpointMessage("/ backward");
  server.send(HTTP_REQUEST_OK);

  ledcWrite(ledChannel1, speed);
  ledcWrite(ledChannel4, speed);
  ledcWrite(ledChannel5, speed);
  ledcWrite(ledChannel8, speed);
  delay(stepLenghtForward);
  ledcWrite(ledChannel1, 0);
  ledcWrite(ledChannel4, 0);
  ledcWrite(ledChannel5, 0);
  ledcWrite(ledChannel8, 0);
}

void handleRight() {
  logEndpointMessage("/ right");
  server.send(HTTP_REQUEST_OK);

  ledcWrite(ledChannel1, speed);
  ledcWrite(ledChannel3, speed);
  ledcWrite(ledChannel5, speed);
  ledcWrite(ledChannel7, speed);
  delay(stepLenghtLeft);
  ledcWrite(ledChannel1, 0);
  ledcWrite(ledChannel3, 0);
  ledcWrite(ledChannel5, 0);
  ledcWrite(ledChannel7, 0);
}


void handleLeft() {
  logEndpointMessage("/ left");
  server.send(HTTP_REQUEST_OK);

  ledcWrite(ledChannel2, speed);
  ledcWrite(ledChannel4, speed);
  ledcWrite(ledChannel6, speed);
  ledcWrite(ledChannel8, speed);
  delay(stepLenghtRight);
  ledcWrite(ledChannel2, 0);
  ledcWrite(ledChannel4, 0);
  ledcWrite(ledChannel6, 0);
  ledcWrite(ledChannel8, 0);
}


void handleLight() {
  logEndpointMessage("/ light");
  server.send(HTTP_REQUEST_OK);

  lightOn = !lightOn;
  switchLight();
}


void handleSwitchOffDelay() {
  logEndpointMessage("/ stepLenght");

  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    logAddMessage(String(i) + " ");  //print id
    logAddMessage("\"" + argumentName + "\" ");  //print name
    logMessage("\"" + argumentValue + "\"");  //print value

    if (argumentName == "f") {
      stepLenghtForward = server.arg(i).toInt();
    }

    if (argumentName == "b") {
      stepLenghtBackward = server.arg(i).toInt();
    }

    if (argumentName == "l") {
      stepLenghtLeft = server.arg(i).toInt();
    }

    if (argumentName == "r") {
      stepLenghtRight = server.arg(i).toInt();
    }

    if (argumentName == "speed") {
      float speedValue = argumentValue.toFloat();
      // do some magic with the value
      speed = speedValue;
    }
  }

  showRootLog = false;
  handleRoot();
  showRootLog = true;
}


void handleRoot() {
  if (showRootLog) {
    logEndpointMessage("/ root");
  }

  String paragraph = String("<p>");
  String lineBreak = String("<br>");

  String stepLenghtText =  String("Step lenght in ms:")
                           + paragraph
                           + String("Forward ") + stepLenghtForward
                           + String(" Backward ") + stepLenghtBackward
                           + lineBreak
                           + String("Left ") + stepLenghtLeft
                           + String(" Right ") + stepLenghtRight
                           + lineBreak
                           + String("(donut is around 2175)")
                           + lineBreak
                           + String("Speed(0-255): ") + String(speed)
                           + paragraph;

  String buttons = lineBreak
                   + getButton("forward") + getButton("backward")
                   + lineBreak
                   + getButton("left") + getButton("right")
                   + lineBreak
                   + getButton("light") + boolToString(lightOn)
                   + lineBreak;

  String endpoints = textToAhref("/forward") + paragraph
                     + textToAhref("/backward") + paragraph
                     + textToAhref("/right") + paragraph
                     + textToAhref("/left") + paragraph
                     + textToAhref("/stepLenght?f=500&b=500&l=100&r=100&speed=100") + paragraph;

  String style = String("<style>body {background-color: black;color: white;}</style>");

  String title = String("<title>ESP32 RC Car</title>");
  String head = String("<head>") + style + title + String("</head>");
  String body = String("<body>")
                + String("<h1>Car over WiFi!</h1>") + paragraph + lineBreak
                + stepLenghtText + paragraph
                + lineBreak
                + buttons + paragraph
                + lineBreak
                + String("This is a super simple website, running on an ESP8266 based micro controller :)</p>") + paragraph
                + String("You can control four motors mounted to RC car model. Each pin is mapped to backward/forwards fuction!</p>") + paragraph
                + lineBreak
                + String("Or you call any <b>endpoint</b> via browser or an app:") + paragraph + lineBreak
                + endpoints + paragraph + lineBreak
                + paragraph + lineBreak
                + String("</i></body>");

  String rootContent = String("<html>") + head + body + String("</html>");

  server.send(HTTP_REQUEST_OK, "text/html", HTML);
}


void handleNotFound() {
  logMessage("404 - " + server.uri());
  logAddMessage(String(millis()));

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

  server.send(HTTP_REQUEST_PNF, "text/plain", message);
}


void switchLight() {
  if (lightOn) {
    digitalWrite(PIN_LIGHT, HIGH);
  } else {
    digitalWrite(PIN_LIGHT, LOW);
  }
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


void addLineToDisplay(String message) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println(message);
  display.display();
}


void logEndpointMessage(String enpointName) {
  Serial.println("---------------------------");
  logMessage(enpointName + "  called by");
  logAddMessageLine(getClientIp());
  logAddMessageLine(" ");
  logAddMessageLine("RC IP: " + ip2Str(WiFi.localIP()));
}


String getButton(String function) {
  return textToButton(function, getUrl(function));
}


String getUrl(String function) {
  String localIp = ip2Str(WiFi.localIP());
  return String("http://") + localIp + String("/") + function;
}


String boolToString(bool b) {
  return b ? "ON" : "OFF";
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


void logAddMessageLine(String text) {
  Serial.println(text);
  if (displayReady) {
    addLineToDisplay(text);
  }
}


String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}
