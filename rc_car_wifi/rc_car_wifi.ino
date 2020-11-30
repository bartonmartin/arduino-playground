#include <ESP8266WebServer.h>

#define PIN_1             D1
#define PIN_2             D2
#define PIN_3             D3
#define PIN_4             D4
#define PIN_5             D5
#define PIN_6             D6
#define PIN_7             D7
#define PIN_8             D8

#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"

//192.168.2.xxx port 80
ESP8266WebServer server(80);

int stepLenghtLeft = 100;
int stepLenghtRight = 100;
int stepLenghtForward = 500;
int stepLenghtBackward = 500;

float speed = 100;


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

  digitalWrite(PIN_1, HIGH);
  digitalWrite(PIN_2, HIGH);
  digitalWrite(PIN_3, HIGH);
  digitalWrite(PIN_4, HIGH);
  digitalWrite(PIN_5, HIGH);
  digitalWrite(PIN_6, HIGH);
  digitalWrite(PIN_7, HIGH);
  digitalWrite(PIN_8, HIGH);

  setupHttpServer();
}


void loop(void) {
  server.handleClient();
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
  server.on("/stepLenght", handleSwitchOffDelay);
  server.onNotFound(handleNotFound);

  // start server
  server.begin();
  Serial.println("HTTP server started");
}

void handleForward() {
  logEndpointMessage("/ forward");

  analogWrite(PIN_1, speed);
  analogWrite(PIN_3, speed);
  analogWrite(PIN_6, speed);
  analogWrite(PIN_8, speed);
  delay(stepLenghtForward);
  digitalWrite(PIN_1, HIGH);
  digitalWrite(PIN_3, HIGH);
  digitalWrite(PIN_6, HIGH);
  digitalWrite(PIN_8, HIGH);

  handleRoot();
}

void handleBackward() {
  logEndpointMessage("/ backward");

  analogWrite(PIN_2, speed);
  analogWrite(PIN_4, speed);
  analogWrite(PIN_5, speed);
  analogWrite(PIN_7, speed);
  delay(stepLenghtBackward);
  digitalWrite(PIN_2, HIGH);
  digitalWrite(PIN_4, HIGH);
  digitalWrite(PIN_5, HIGH);
  digitalWrite(PIN_7, HIGH);

  handleRoot();
}

void handleRight() {
  logEndpointMessage("/ right");

  analogWrite(PIN_2, speed);
  analogWrite(PIN_3, speed);
  analogWrite(PIN_5, speed);
  analogWrite(PIN_8, speed);
  delay(stepLenghtRight);
  digitalWrite(PIN_2, HIGH);
  digitalWrite(PIN_3, HIGH);
  digitalWrite(PIN_5, HIGH);
  digitalWrite(PIN_8, HIGH);

  handleRoot();
}

void handleLeft() {
  logEndpointMessage("/ left");

  analogWrite(PIN_1, speed);
  analogWrite(PIN_4, speed);
  analogWrite(PIN_6, speed);
  analogWrite(PIN_7, speed);
  delay(stepLenghtLeft);
  digitalWrite(PIN_1, HIGH);
  digitalWrite(PIN_4, HIGH);
  digitalWrite(PIN_6, HIGH);
  digitalWrite(PIN_7, HIGH);

  handleRoot();
}


void handleSwitchOffDelay() {
  logEndpointMessage("/ stepLenght");

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

  int httpRequestCode = 200; // OK

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
                           + String("donut is 2175")
                           + paragraph;

  String buttons = lineBreak
                   + getButton("forward") + getButton("backward")
                   + lineBreak
                   + getButton("left") + getButton("right")
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
                + stepLenghtText + paragraph + lineBreak
                + String("This is a super simple website, running on an ESP8266 based micro controller :)</p>") + paragraph
                + String("You can control four motors mounted to RC car model. Each pin is mapped to backward/forwards fuction!</p>") + paragraph
                + lineBreak
                + String("<h3>Tap on buttons right here:</h3>") + paragraph + lineBreak
                + buttons + paragraph + lineBreak
                + String("Or you call any <b>endpoint</b> via browser or an app:") + paragraph + lineBreak
                + endpoints + paragraph + lineBreak
                + paragraph + lineBreak
                + String("</i></body>");
  String rootContent = String("<html>") + head + body + String("</html>");

  server.send(httpRequestCode, "text/html", rootContent);
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


String getButton(String function) {
  return textToButton(function, getUrl(function));
}


String getUrl(String function) {
  String localIp = ip2Str(WiFi.localIP());
  return String("http://") + localIp + String("/") + function;
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
