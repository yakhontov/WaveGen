#include <SoftwareSerial.h>
SoftwareSerial sim800(12, 11); // rx, tx

void setup() {
  Serial.begin(115200);
  sim800.begin(9600);
  delay(1000);
  sim800.println("AT");
  Serial.println("Trying AT");
}

void loop() {
  if(sim800.available())
    Serial.write(sim800.read());
  if(Serial.available())
    sim800.write(Serial.read());
}
