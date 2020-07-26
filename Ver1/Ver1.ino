//#define USEI2C

#include <NewPing.h>

#ifdef USEI2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

class CWave {
  void Meashured(uint16_t dist);
  uint16_t GetTime();
  uint16_t GetHeight();
} wave;

// defines pins numbers
const int trigPin = A1;
const int echoPin = A2;
const int maxDist = 200; // Максимальная дистанция измерения в сантиметрах. Не более 200-400

NewPing sonar(trigPin, echoPin, maxDist);

void clearLine(int line){
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void printDisplay(String message){
  clearLine(1);
  Serial.println(message);
  lcd.setCursor(0, 1);
  lcd.print(message);
}

void setup() {
  Serial.begin(9600);

#ifdef USEI2C
  lcd.init();
  lcd.backlight();
#else
  lcd.begin(16, 2);
#endif
  
  lcd.print("Supervolnometer");
  delay(3000);
  lcd.setCursor(0, 0);
  lcd.print("Distance, cm   ");

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
}

long duration;
int distance;

void loop() {
  //distance = sonar.ping_cm();
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(100);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH, 10000);
  distance = duration / 59;//cm = duration/58.8  
  
  Serial.println(distance);
  printDisplay(String(distance));

  //+delay(50);
}
