#include <SoftwareSerial.h>
SoftwareSerial sim800(2, 3); // rx, tx

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  sim800.println("AT");
}

void loop() {
  if(sim800.available())
    Serial.write(sim800.read());
  if(Serial.available())
    sim800.write(Serial.read());
}
