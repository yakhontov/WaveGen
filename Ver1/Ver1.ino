//#define USEI2C

// book: https://www.alternative-energy-tutorials.com/wave-energy/wave-energy.html

#ifdef USEI2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

class CWave {
  void Meashured(uint16_t _distance);
  uint16_t GetPeriod();
  uint16_t GetHeightCm();
} wave;

// defines pins numbers
const int trigPin = A1;
const int echoPin = A2;
const int maxDist = 200; // Максимальная дистанция измерения в сантиметрах. Не более 200-400

void clearLine(int line){
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void printDisplay(String message){
  clearLine(1);
  //Serial.println(message);
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
int cnt =0;

void loop() {
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(100);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH, 10000);
  distance = duration / 59;//cm = duration/58.8  
  cnt++;
  if(cnt == 1000){
    cnt = 0;
    Serial.println(millis()); // Одно измерение длится 10мс
  }
  
  //Serial.println(distance);
  printDisplay(String(distance));

  //+delay(50);
}
