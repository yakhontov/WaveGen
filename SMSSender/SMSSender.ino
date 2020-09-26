//#define USEI2C

// info:
// https://ru.qwe.wiki/wiki/Wave_power
// https://www.alternative-energy-tutorials.com/wave-energy/wave-energy.html
// https://flot.com/publications/books/shelf/vasiliev/18.htm

#ifdef USEI2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

//#define DEBUGWAVE
#define SMS_TARGET  "+380501234567"

const int trigPin = A1;
const int echoPin = A2;
const int maxDist = 200; // Максимальная дистанция измерения в сантиметрах. Не более 200-400

#include "MedianTemplate.h"

// Через этот быстрый фильтр проходят все значения расстояния чтобы отсеять ошибочные измерения
MedianFilter<uint16_t, 7> inputAmpFilter(0);

// Этот фильтр работает как очередь для нахождения максимального и минимального значения расстояния
// Период его работы должен быть больше периода волны. Для увеличения периода работы можно его сделать вложенным
MedianFilter<uint16_t, 250> minmaxAmpQueue(0);

// Через этот фильтр проходят все значения периода чтобы отсеять ошибочные измерения
MedianFilter<uint32_t, 5> inputPerFilter(0);

#define TINY_GSM_MODEM_SIM800

#define SerialMon Serial

#include <SoftwareSerial.h>
SoftwareSerial SerialAT(12, 11);  // RX, TX

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

void setup() {
  SerialMon.begin(115200);
  delay(10);
  
#ifdef USEI2C
  lcd.init();
  //lcd.backlight();
#else
  lcd.begin(16, 2);
#endif

  DBG("Wait for modem start...");
  lcd.setCursor(0, 0);
  lcd.print("-= VOLNOREZZZ =-");
  delay(3000);

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  SerialAT.begin(9600);
  DBG("Initializing modem...");
  if (!modem.restart())
    DBG("Failed to restart modem");
  else {
    String name = modem.getModemName();
    DBG("Modem Name:", name);
    String modemInfo = modem.getModemInfo();
    DBG("Modem Info:", modemInfo);
  }
}

int GetDistance(){
  long duration;
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(100);//100
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH, maxDist * 59);
  return duration / 59; //cm = duration/58.8;
}

// Вызывается каждый раз, когда из ультразвукового давтчика получено значение
// Аргумент - измеренное расстояние в миллиметрах 
void Measured(uint16_t dist) {
  static uint32_t upperThirdTime = 0; // Время последего перехода волны в верхнюю треть амплитуды
  static bool waitingForUp = true; // Флаг ожидания перехода волны в верхнюю треть амплитуды
  
  dist = inputAmpFilter.putGet(dist); // Отфильтровываем мусор
  minmaxAmpQueue.put(dist); // Заполняем очередь для нахождения мин/макс
  dist -= minmaxAmpQueue.getMin(); // Вычитаем из текущего значения минимальное чтобы привести к общей базе для удобства расчетов
  uint16_t amp2 = minmaxAmpQueue.getMax() - minmaxAmpQueue.getMin(); // Размах волны (двойная амплитуда)
  if(waitingForUp) { // Ожидаем переход в верхнюю треть
    if(dist > amp2 * 2 / 3) { // Переход в верхнюю треть произошел
      uint32_t t = millis(); // Определяем время перехода
      inputPerFilter.put(t - upperThirdTime); // Отправляем вычисленное значение периода в фильтр
      upperThirdTime = t; // Запоминаем время перехода
      waitingForUp = false; // Сбрасываем флаг ожидания перехода волны в верхнюю треть амплитуды (ожидаем переход в нижнюю треть)
    }
  } else { // Ожидаем переход в нижнюю треть
    if(dist < amp2 / 3) // Переход в нижнюю треть произошел
      waitingForUp = true; // Выставляем флаг ожидания перехода волны в верхнюю треть амплитуды
  }
}

void loop() {
  static int dist = 0;
  
  static long getDistPeriod = 30;
  static long getDistAt = millis();
  if(getDistAt <= millis()) {
    getDistAt = millis() + getDistPeriod;

#ifdef DEBUGWAVE
    static float s = 0;
    const float si = 0.05;
    dist = sin(s) * 100 + 150 + random(-20, 20);
    s += si;
#else
    dist = GetDistance();
#endif
    Measured(dist);
  }

  static long updScrPeriod = 500;
  static long updScrAt = millis();
  if(updScrAt <= millis()) {
    updScrAt = millis() + updScrPeriod;
    // В верхнюю строку экрана выводим следующие оперативные данные: последняя измеренная дистанция, она же после фильтра, амплитуда, период
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print(dist);
    lcd.setCursor(4, 0);
    lcd.print(inputAmpFilter.get() );
    lcd.setCursor(8, 0);
    lcd.print(minmaxAmpQueue.getMax() - minmaxAmpQueue.getMin());
    lcd.setCursor(12, 0);
    lcd.print(inputPerFilter.get());
  }

  const uint32_t sendsmsPeriod = 60000;
  static uint32_t nextSmsAt = millis() + 15000;
  if(millis() >= nextSmsAt) {
    nextSmsAt = millis() + sendsmsPeriod;

    // В нижнюю строку экрана выводим данные, которые отправлены в последнем СМС: высота волны, амплитуда волны, период волны, длина волны
    int H = minmaxAmpQueue.getMax() - minmaxAmpQueue.getMin();
    int A = H / 2;
    int T = inputPerFilter.get();
    float l = pow(float(T) / 1000.0, 2) * 9.8 / (2 * PI) * 100.0; // cm
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print((int)l);
    lcd.setCursor(8, 1);
    lcd.print(H);
    lcd.setCursor(12, 1);
    lcd.print(T);

    String sms = "t=" + String(millis()/1000) + "s, H=" + H + "cm, A=" + A + "cm, T=" + T + "ms, l=" + (int)l + "cm";
    Serial.println("SMS :" + sms);
    int res = modem.sendSMS(SMS_TARGET, sms);
    DBG("SMS:", res ? "OK" : "fail");
  }
}
