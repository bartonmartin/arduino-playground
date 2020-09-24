#include <WEMOS_SHT3X.h>


#define PIN_D1 D1
#define PIN_D2 D2

SHT3X sht30(0x45);

void setup() {

  Serial.begin(115200);

}

void loop() {


  Serial.println(String("pin 1: " )+ String(D1));
  Serial.println(String("pin 2: " )+ String(D2));
  
  if(sht30.get()==0){
    Serial.print("Temperature in Celsius : ");
    Serial.println(sht30.cTemp);
    Serial.print("Temperature in Fahrenheit : ");
    Serial.println(sht30.fTemp);
    Serial.print("Relative Humidity : ");
    Serial.println(sht30.humidity);
    Serial.println();
  }
  else
  {
    Serial.println("Error!");
  }
  delay(1000);

}
