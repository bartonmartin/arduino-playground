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


void setup(void) {
  // setup serial console
  Serial.begin(115200);

  // value LOW == motor is on
  // value HIGH == motor is off
  // PIN_1 LOW - front right forward
  // PIN_2 LOW - front right backward
  // PIN_3 LOW - front left forward
  // PIN_4 LOW - front left backward
  // PIN_5 LOW - rear right backward
  // PIN_6 LOW - rear right forward
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
  Serial.println("HTTP server started");
}

void handleForward() {
  logMessage("/ forward");

  digitalWrite(PIN_1, LOW);
  digitalWrite(PIN_3, LOW);
  digitalWrite(PIN_6, LOW);
  digitalWrite(PIN_8, LOW);
  delay(stepLenghtForward);
  digitalWrite(PIN_1, HIGH);
  digitalWrite(PIN_3, HIGH);
  digitalWrite(PIN_6, HIGH);
  digitalWrite(PIN_8, HIGH);

  handleRoot();
}

void handleBackward() {
  logMessage("/ backward");

  digitalWrite(PIN_2, LOW);
  digitalWrite(PIN_4, LOW);
  digitalWrite(PIN_5, LOW);
  digitalWrite(PIN_7, LOW);
  delay(stepLenghtBackward);
  digitalWrite(PIN_2, HIGH);
  digitalWrite(PIN_4, HIGH);
  digitalWrite(PIN_5, HIGH);
  digitalWrite(PIN_7, HIGH);

  handleRoot();
}

void handleRight() {
  logMessage("/ right");

  digitalWrite(PIN_2, LOW);
  digitalWrite(PIN_3, LOW);
  digitalWrite(PIN_5, LOW);
  digitalWrite(PIN_8, LOW);
  delay(stepLenghtRight);
  digitalWrite(PIN_2, HIGH);
  digitalWrite(PIN_3, HIGH);
  digitalWrite(PIN_5, HIGH);
  digitalWrite(PIN_8, HIGH);

  handleRoot();
}


void handleLeft() {
  logMessage("/ left");

  digitalWrite(PIN_1, LOW);
  digitalWrite(PIN_4, LOW);
  digitalWrite(PIN_6, LOW);
  digitalWrite(PIN_7, LOW);
  delay(stepLenghtLeft);
  digitalWrite(PIN_1, HIGH);
  digitalWrite(PIN_4, HIGH);
  digitalWrite(PIN_6, HIGH);
  digitalWrite(PIN_7, HIGH);

  handleRoot();
}


void handleLight() {
  logMessage("/ light");
  lightOn = !lightOn;
  switchLight();
  handleRoot();
}


void handleSwitchOffDelay() {
  logMessage("/ stepLenght");

  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    Serial.print(String(i) + " ");  //print id
    Serial.print("\"" + argumentName + "\" ");  //print name
    Serial.println("\"" + argumentValue + "\"");  //print value

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

  handleRoot();
}


void handleRoot() {
  Serial.println("");
  Serial.println("someone is calling root");

  String paragraph = String("<p>");
  String lineBreak = String("<br>");

  String stepLenghtText =  String("Step lenght in ms:")
                           + paragraph
                           + String("Forward ") + stepLenghtForward
                           + String(" Backward ") + stepLenghtBackward
                           + paragraph
                           + String("Left ") + stepLenghtLeft
                           + String(" Right ") + stepLenghtRight
                           + paragraph
                           + String("(donut is around 2175)")
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

  server.send(HTTP_REQUEST_OK, "text/html", rootContent);
}


void handleNotFound() {
  logMessage("404 - " + server.uri());

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


String getButton(String function) {
  return textToButton(function, getUrl(function));
}


String getUrl(String function) {
  String localIp = ip2Str(WiFi.localIP());
  return String("http://") + localIp + String("/") + function;
}


void logMessage(String text) {
  Serial.println(text);
  if (displayReady) {
    showOnDisplay(text);
  }
}


String getClientIp() {
  return server.client().remoteIP().toString();
}


String textToAhref(String text) {
  String ahref = String("<a href=\"") + text + String("\">") + ip2Str(WiFi.localIP()) + text + String("</a>");
  Serial.println(ahref);
  return ahref;
}

String textToButton(String text, String url) {
  String endpointText = ip2Str(WiFi.localIP()) + url;
  String ahref = String("<a href=\"") + url + String("\"><button>   ") + text  + String("   </button></a> ");
  Serial.println(String("Endpoint: ") + endpointText);
  return ahref;
}


String boolToString(bool b) {
  return b ? "ON" : "OFF";
}


String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}
