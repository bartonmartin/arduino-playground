// custom HTTP server controlling RGB strip and 4000k LED strip with simple MOSFET driver
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>

// custom pins definition
#define PIN_LED             D2
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"


// number of RGB leds in the custom strip
int leds = 37;

//  server will run on something like 192.168.2.xxx port 80
ESP8266WebServer server(80);

// RGB LED library to control the individually addressable RGB pixels
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(leds, PIN_LED, NEO_GRB + NEO_KHZ800);


int r = 0;
int g = 0;
int b = 0;
int white = 0;
int limit = 80; // at 80 the max current draw does not go above 500mA - limit of D1 mini usb port
int pixelTestAnimationDelay = 500;


int led = D0;           // the PWM pin the LED is attached to
int brightness = 0;    // how bright the LED is
int fadeAmount = 1;    // how many points to fade the LED by

boolean fadeIn = true;


void setup(void) {
  // serial monitor will use 115200 baud
  Serial.begin(115200);

  // setup all IO pins
  pinMode(led, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // onboard LED works on inverted logic - in this case HIGH is off, LOW is on
  digitalWrite(LED_BUILTIN, HIGH);

  // setup wifi mode, network name and password
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");

  // wait for wifi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // available endpoints
  server.on("/", handleRoot);
  server.on("/turnOff", handleTurnOff);
  server.on("/turnOn", handleTurnOn);
  server.on("/rgb", handleRgb);
  server.on("/rgbSimple", handleRgbSimple);
  server.on("/random", handleRandom);
  server.on("/blinker", handleBlinker);
  server.on("/flash", handleFlash);
  server.onNotFound(handleNotFound);

  // start the http server
  server.begin();
  Serial.println("HTTP server started");

  // start RGB LED
  pixels.begin();

  // run initial test
  runPixelTest();
  runLedFadeTest();
}


void loop(void) {
  server.handleClient();
}

void runPixelTest() {
  // first and last pixel of the strip will blink with each color
  pixels.setPixelColor(0, pixels.Color(1, 0, 0));
  pixels.setPixelColor(leds - 1, pixels.Color(1, 0, 0));
  pixels.show();
  delay(pixelTestAnimationDelay);
  pixels.setPixelColor(0, pixels.Color(0, 1, 0));
  pixels.setPixelColor(leds - 1, pixels.Color(0, 1, 0));
  pixels.show();
  delay(pixelTestAnimationDelay);
  pixels.setPixelColor(0, pixels.Color(0, 0, 1));
  pixels.setPixelColor(leds - 1, pixels.Color(0, 0, 1));
  pixels.show();
  delay(pixelTestAnimationDelay);
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.setPixelColor(leds - 1, pixels.Color(0, 0, 0));
  pixels.show();
  delay(pixelTestAnimationDelay);
}

void runLedFadeTest() {
  // set the brightness of pin 9:
  for (int i = 0; i > 200; i++) {
    analogWrite(led, brightness);

    // change the brightness for next time through the loop:
    brightness = brightness + fadeAmount;

    // reverse the direction of the fading at the ends of the fade:
    if (brightness <= 0 || brightness >= 255) {
      fadeAmount = -fadeAmount;
    }
    // wait for 30 milliseconds to see the dimming effect
    if (brightness != 0) {
      if (brightness > 100) {
        delay(2);
      } else {
        delay(5);
      }
    } else {
      digitalWrite(led, LOW);
      delay(1000);
    }
  }
}


int checkValue(int value) {
  if (value > limit) {
    return limit;
  }
  if (value < 0) {
    return 0;
  }
  return value;
}


String getButton(String color, String function) {
  String text = "";
  String url = "";
  int value = 0;

  if (function == String("+")) {
    if (color == String("R")) {
      value = checkValue(r + 1);
      url = getUrl(value, g, b);
    } else if (color == String("G")) {
      value = checkValue(g + 1);
      url = getUrl(r, value, b);
    } else if (color == String("B")) {
      value = checkValue(b + 1);
      url = getUrl(r, g, value);
    }

    text = String(color) + String(" +1");
  } else {
    if (color == "R") {
      value = checkValue(r - 1);
      url = getUrl(value, g, b);
    } else if (color == String("G")) {
      value = checkValue(g - 1);
      url = getUrl(r, value, b);
    } else if (color == String("B")) {
      value = checkValue(b - 1);
      url = getUrl(r, g, value);
    }

    text = String(color) + String(" -1");
  }

  return textToButton(text, url);
}


String getUrl(int red, int green, int  blue) {
  String localIp = ip2Str(WiFi.localIP());
  return String("http://") + localIp + String("/rgb?r=") + red + String("&g=") + green + String("&b=") + blue;
}


void handleRoot() {
  Serial.println("");
  Serial.println("someone is calling root");

  int httpRequestCode = 200; // OK

  String paragraph = String("<p>");
  String lineBreak = String("<br>");
  String buttons = getButton(String("R"), String("+")) + String(" ")
                   + getButton(String("R"), String("-")) + paragraph
                   + getButton(String("G"), String("+")) + String(" ")
                   + getButton(String("G"), String("-")) + paragraph
                   + getButton(String("B"), String("+")) + String(" ")
                   + getButton(String("B"), String("-")) + paragraph
                   + getButton(String("W"), String("+")) + String(" ")
                   + getButton(String("W"), String("-")) + paragraph;

  String endpoints = textToAhref("/turnOn") + paragraph
                     + textToAhref("/turnOff") + paragraph
                     + textToAhref("/random") + paragraph
                     + textToAhref("/blinker") + paragraph
                     + textToAhref("/flash") + paragraph
                     + textToAhref("/rgb?r=1&g=0&b=0&w=0&leds=37") + paragraph;

  String style = String("<style>body {background-color: black;color: white;}</style>");

  String rgbTextStyle = String("<p style =\"background-color:rgb(") + rgbValue(r) + String(",") + rgbValue(g) + String(",") + rgbValue(b) + String(")\">");

  String title = String("<title>ESP8266 RGB server</title>");
  String head = String("<head>") + style + title + String("</head>");
  String body = String("<body>")
                + String("<h1>") + String("RGB \(") + r  + String(",") + g + String(",") + b + String("\)") + String(" POWER</h1>") + paragraph
                + lineBreak
                + String("This is a super simple website, running on an ESP8266 based micro controller :)") + paragraph
                + String("You can change color of WS2812B LED strip with 37 diodes !") + paragraph
                + String("You an change brightness of 2835 4000k 12B LED strip with PWM dimming using FDP6030BL MOSFET!") + paragraph
                + lineBreak
                + String("<h3>Tap on buttons right here:</h3>") + paragraph + lineBreak
                + rgbTextStyle + String("LED strip length: ") + leds + String("</p>")
                + buttons + paragraph + lineBreak
                + String("Or you call any <b>endpoint</b> via browser or an app:") + paragraph + lineBreak
                + endpoints + paragraph + lineBreak
                + paragraph + lineBreak
                + String("</i></body>");
  String rootContent = String("<html>") + head + body + String("</html>");

  server.send(httpRequestCode, "text/html", rootContent);
}


String rgbValue(int value) {
  return String("") + value * 2;
}


void handleTurnOn() {
  logEndpointMessage("/turnOn");

  int httpRequestCode = 200; // OK
  digitalWrite(LED_BUILTIN, LOW);

  r = 5;
  g = 5;
  b = 5;
  white = 5;

  setStripColor(r, g, b, 10);
  analogWrite(led, white);
  handleRoot();
}


void handleTurnOff() {
  logEndpointMessage("/turnOff");

  int httpRequestCode = 200; // OK
  digitalWrite(LED_BUILTIN, HIGH);

  r = 0;
  g = 0;
  b = 0;
  white = 0;

  setStripColor(r, g, b, 10);
  digitalWrite(led, LOW);
  handleRoot();
}

void handleRgb() {
  handleColorChange(true);
}

void handleRgbSimple() {
  handleColorChange(false);
}


void handleColorChange(boolean showRoot) {
  logEndpointMessage("/rgb");

  if (!showRoot) {
    int httpRequestCode = 200; // OK
    server.send(200, "text/plain", "ok");
  }

  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    Serial.print(String(i) + " ");  //print id
    Serial.print("\"" + argumentName + "\" ");  //print name
    Serial.println("\"" + argumentValue + "\"");  //print value

    if (argumentName == "r") {
      r = checkValue(server.arg(i).toInt());
    }

    if (argumentName == "g") {
      g = checkValue(server.arg(i).toInt());
    }

    if (argumentName == "b") {
      b = checkValue(server.arg(i).toInt());
    }

    if (argumentName == "w") {
      white = server.arg(i).toInt();
    }

    if (argumentName == "leds") {
      leds = server.arg(i).toInt();
      pixels.updateLength(leds);
    }
  }

  Serial.println(String("White:") + white);
  if (white == 0) {
    digitalWrite(led, LOW);
  } else {

    if (white > 255) {
      white = 255;
    }

    if (white == 255) {
      digitalWrite(led, HIGH);
    } else {
      analogWrite(led, white);
    }
  }


  for (int i = 0; i < leds; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();
    delay(10);
  }

  if (showRoot) {
    handleRoot();
  }
}

void handleRandom() {
  logEndpointMessage("/random");

  server.send(200, "text/plain", "ok");
  for (int i = 0; i < leds; i++) {
    pixels.setPixelColor(i, pixels.Color(random(0, 20), random(0, 20), random(0, 20)));
    pixels.show();
    delay(10);
  }
}


void handleBlinker() {
  logEndpointMessage("/blinker");

  int httpRequestCode = 200; // OK
  server.send(httpRequestCode, "text/plain", "ok");

  setStripColor(9, 2, 0, 50);
  delay(400);

  setStripColor(0, 0, 0, 1);
  delay(400);
}


void handleFlash() {
  logEndpointMessage("/flash");

  int httpRequestCode = 200; // OK
  server.send(httpRequestCode, "text/plain", "ok");

  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, white);
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


void setStripColor(int r, int g, int b, int pixelAnimationDelay) {
  for (int i = 0; i < leds; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();
    delay(pixelAnimationDelay);
  }
}


void logEndpointMessage(String enpointName) {
  Serial.println("---------------------------");
  Serial.println(getClientIp() + " vola " + enpointName);
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



String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}
