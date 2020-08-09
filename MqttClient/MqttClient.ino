//#define USEI2C

// book: https://www.alternative-energy-tutorials.com/wave-energy/wave-energy.html

#ifdef USEI2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
#else
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

const int trigPin = A1;
const int echoPin = A2;
const int maxDist = 200; // Максимальная дистанция измерения в сантиметрах. Не более 200-400

#define TINY_GSM_MODEM_SIM800

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
//#define SerialAT Serial1

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3); // RX, TX

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Range to attempt to autobaud
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 9600

// Add a reception delay - may be needed for a fast processor at a slow baud rate
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// MQTT details
// http://www.hivemq.com/demos/websocket-client/
const char* broker = "broker.hivemq.com";

//const char* topicLed = "GsmClientTest/led";
//const char* topicInit = "GsmClientTest/init";
//const char* topicLedStatus = "GsmClientTest/ledStatus";
const char* topicWGTime = "WaveGen/time";
const char* topicWGInit = "WaveGen/started";

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient mqtt(client);

uint32_t lastReconnectAttempt = 0;

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  // Only proceed if incoming message's topic matches
//  if (String(topic) == topicLed) {
//    ledStatus = !ledStatus;
//    digitalWrite(LED_PIN, ledStatus);
//    mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
//  }
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  // Connect to MQTT Broker
  boolean status = mqtt.connect("GsmClientTest");

  // Or, if you want to authenticate MQTT:
  //boolean status = mqtt.connect("GsmClientName", "mqtt_user", "mqtt_pass");

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  mqtt.publish(topicWGInit, "WaveGen started");
  //mqtt.subscribe(topicLed);
  return mqtt.connected();
}

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
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);
  
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

  //pinMode(LED_PIN, OUTPUT);

  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  // !!!!!!!!!!!

  SerialMon.println("Wait...");

  // Set GSM module baud rate
  TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  // SerialAT.begin(9600);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
    modem.simUnlock(GSM_PIN);
  }
#endif

#if TINY_GSM_USE_WIFI
    // Wifi connection parameters must be set before waiting for the network
  SerialMon.print(F("Setting SSID/password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    SerialMon.println(" success");

  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
  }
#endif

  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

int GetDistance(){
  long duration;
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(100);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH, maxDist * 59);
  return duration / 59; //cm = duration/58.8;
}

const int maxArr = 256;
uint8_t arr[maxArr];
int top = 0;

void SetDistance(uint8_t d) {
  if(!d)
    return;
  arr[top++] = d;
  if(top >= maxArr)
    top = 0;
}

uint8_t GetArr(int i) { // Возвращает элемент массива. Для упрощения доступа к кольцевому массиву
  if(i < 0 || i>= maxArr)
    return 0;
  i += top;
  if(i >= maxArr)
    i -= maxArr;
    return arr[i];
}

uint8_t GetMedian(int i) { // По смыслу как и GetArr. Но выдает медианный результат для элемента массива i и двух ближайших его соседей
  uint8_t a = GetArr(i - 1);
  uint8_t b = GetArr(i);
  uint8_t c = GetArr(i + 1);
  if((a <= b && b <= c) || (a >= b && b >= c)) return b;
  if((b <= a && a <= c) || (b >= a && a >= c)) return a;
  if((a <= c && c <= b) || (a >= c && c >= b)) return c;
}

int GetAmplitude() { // Просеиваем весь массив и находим амплитуду
  int _min = 256;
  int _max = -1;
  int a;
  for(int i = 0; i < maxArr; i++){
    a = GetMedian(i);
    if(a < _min)
      _min = a;
    if(a > _max)
      _max = a;
  }
  return _max - _min;
}

int GetPeriod() {
  
}

void loop() {
  static int distance;
  static long getDistPeriod = 30;
  static long getDistAt = millis();
  if(getDistAt <= millis()) {
    getDistAt = millis() + getDistPeriod;
    distance = GetDistance();
  }

  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(20);
    return;
  }

  static long publishTimePeriod = 1000;
  static long publishTimeAt = millis();
  if(publishTimeAt <= millis()) {
    publishTimeAt = millis() + publishTimePeriod;
    String s = String(millis()) + " " + String(distance);
    mqtt.publish(topicWGTime, s.c_str());
    Serial.println(s);
  }
  
  mqtt.loop();
}
