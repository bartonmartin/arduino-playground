#include <WEMOS_Matrix_LED.h>

MLED mled(5); //set intensity=5
int x = 0;
int y = 0;

int pinValue = LOW;

void setup() {
  // put your setup code here, to run once:

  pinMode(D1, OUTPUT);

  x = 0;
  y = 0;
}

void loop() {
  if (x == 8 && y == 7) {
    for (int i = 0; i < 8; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        mled.dot(i, j, 0); //clear dot
        mled.display();
        x = 0;
        y = 0;
      }
    }
    pinValue = pinValue == HIGH ? LOW : HIGH;
    digitalWrite(D1, pinValue);
  } else {
    mled.dot(x, y); // draw dot
    mled.display();
    delay(50);

    if (x == 8) {
      y++;
      x = 0;
    } else {
      x++;
    }
  }
}
