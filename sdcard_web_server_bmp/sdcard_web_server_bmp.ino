// wifi network stuff
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

// sd card read-write
#include <SPI.h>
#include <SD.h>

// bmp sensor
#include <Wire.h>
#include <Adafruit_BMP085.h>


// wifi network stuff
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"
WiFiServer server(80);
ESP8266WiFiMulti wiFiMulti;


// sd card read-write
#define SD_CS_PIN SS
File myFile;
int status = WL_IDLE_STATUS;


// bmp sensor
Adafruit_BMP085 bmp;

long latestUnixTimeStamp = 0;


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  // setup sd card
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");


  // setup bmp sensor
  //Wire.begin (4, 5);
  if (!bmp.begin()) {
    Serial.println("Could not find BMP180 or BMP085 sensor at 0x77");
  }


  // http server
  Serial.println("starting wifi init");
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }


  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("init done");
  server.begin();


  // wifi settings
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");


  // you're connected now, so print out the status:
  printWifiStatus();


  // send simple handshake to router (192.168.1.2)
  // router will send current datetime in response
  // cache datetime info from network
  // add current millis() to datetime from router and save it with received data from sensor
  // datetime cache should refresh once per hour (1000*60*60ms)
  // data on SD card are saved in a file named "YYYY/MM/DD/HH"

  sendHttpRequest();
}


void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();

  if (client) {
    Serial.println("new client");

    float temperatureValue = bmp.readTemperature();
    int pressureValue = bmp.readPressure();

    // an http request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        String line = client.readStringUntil('\r');
        String keywordStart = "GET /";
        String keywordEnd = " HTTP";
        int positionStart = line.indexOf(keywordStart) + keywordStart.length();
        int positionEnd = line.indexOf(keywordEnd);

        if (positionStart != -1 && positionEnd != -1) {
          String urlParams = line.substring(positionStart, positionEnd);
          Serial.println("*");
          Serial.println("*");
          Serial.print("urlParams: ");
          Serial.print(urlParams);
          Serial.println("-it fucking works");
          Serial.println("*");
          Serial.println("*");


          // open the file. note that only one file can be open at a time,
          // so you have to close this one before opening another.
          myFile = SD.open("test.txt", FILE_WRITE);


          // if the file opened okay, write to it:
          if (myFile) {
            Serial.print("Writing to test.txt...");
            Serial.println(urlParams);
            long rightNow = latestUnixTimeStamp + (millis() / 1000);
            //myFile.print("unix-timestamp:");
            myFile.print(rightNow);
            myFile.print(", ");
            //myFile.print("data:");
            myFile.print(urlParams);
            myFile.print(", ");
            myFile.print(temperatureValue);
            myFile.print(", ");
            myFile.print(pressureValue);
            myFile.print("\n");

            // close the file:
            myFile.close();
            Serial.println("done.");
          } else {
            // if the file didn't open, print an error:
            Serial.println("error opening test.txt");
          }
        }
        Serial.println(line);


        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");


          String temperature = String("<b>Temperature = </b>") + temperatureValue + String(" Celsius");
          String pressure = String("<b>Pressure = </b>") + pressureValue + String(" Pascal");


          // re-open the file for reading:
          myFile = SD.open("test.txt");
          if (myFile) {
            client.println(temperature);
            client.print("<br>");
            client.println(pressure);
            client.print("<br>");
            client.print("<br>");
            client.println("<h3>Data saved on SD card:</h3>");
            client.print("<br>");
            client.println("<b>unix-timestamp</b>:Long [seconds]<b>,</b> <b>data</b>:String [text],</b> <b>temperature</b>:float [Celsius],</b> <b>pressure</b>:int [pascal]");
            client.print("<br>");
            client.print("<br>");


            // read from the file until there's nothing else in it:
            while (myFile.available()) {
              String character = String((char)myFile.read());
              Serial.print(character);

              if (character == "\n") {
                client.print("<br>");
                client.println("<p>");
              } else {
                client.print(character);
              }
            }
            // close the file:
            myFile.close();
          } else {
            // if the file didn't open, print an error:
            client.println("error opening test.txt");
          }

          client.println("</html>");
          break;
        }


        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }

    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();

    Serial.println("client disonnected");
  }
}


void sendHttpRequest() {
  Serial.println("sendHttpRequest");
  if ((wiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;

    HTTPClient http;
    String url = String("http://worldtimeapi.org/api/timezone/Europe/Prague");

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, url)) {  // HTTP

      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();
      http.GET();
      http.headers();

      for (int i = 0; i < http.headers(); i++) {
        Serial.print("header: ");
        Serial.println(http.header(i));
      }

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();

          String keywordStart = "\"unixtime\":";
          String keywordEnd = ",\"utc_datetime\":";
          int positionStart = payload.indexOf(keywordStart) + keywordStart.length();
          int positionEnd = payload.indexOf(keywordEnd);


          if (positionStart != -1 && positionEnd != -1) {
            String timeInUnix = payload.substring(positionStart, positionEnd);
            Serial.println("*");
            Serial.println("*");
            Serial.print("timeInUnix: ");
            Serial.println(timeInUnix);
            Serial.println("*");
            Serial.println("*");
            int arrayLength = timeInUnix.length() + 1;
            char copy[arrayLength];
            timeInUnix.toCharArray(copy, arrayLength);
            latestUnixTimeStamp = atol(copy);
          } else {
            latestUnixTimeStamp = latestUnixTimeStamp + (millis() / 1000);
          }


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


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
