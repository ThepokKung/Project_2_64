/* Libary */
#include <SoftwareSerial.h>
#include <DHT.h>
#include <Arduino.h>
#include <Wire.h>
//#include <BMP180I2C.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

/* Define */
#define DHTPIN D1
#define DHTTYPE DHT11
#define BMP_I2C_ADD 0x77
#define Pull_But D5

/*  */
DHT dht(DHTPIN, DHTTYPE);
//BMP180I2C bmp180(BMP_I2C_ADD);
SoftwareSerial mySerial(D2, D3); // RX, TX
WiFiClient espClient;
PubSubClient mqtt(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* Global Value */
unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
unsigned int pm10 = 0;
float dht_hum = 0;
float dht_temp = 0;
float bmp_temp = 0;
float bmp_press = 0;
/* Check Val */
int check_time;

/* Json config */
StaticJsonDocument<256> doc;
char buffer[256];

/* Devide Id */
String deviceid = "kkn0001";

/* MQTT Connect */
const char* mqttServer = "kraiwich.thddns.net";
const int mqttPort = 4441;
const char* mqttUser = "kraiwich";
const char* mqttPassword = "kraiwich1234";
char mqtt_name_id[] = "";
const char* pubTopic = "kraiwich/project";

void setup() {
  Serial.begin(9600);
  lcd.begin();
  lcd.backlight();
  lcd.print("Welcome to PPM");
  lcd.setCursor(0, 1);
  lcd.print("Waiting    Setup");
  while (!Serial) ;
  mySerial.begin(9600);
  pinMode(Pull_But, INPUT_PULLUP);
  Setup_WM();
  dht.begin();
  //Setup_BMP();
  MQTT_connect();
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Setup  Finsh....");
  delay(500);
  lcd.clear();
}

void loop() {
  /* MQTT Check Connect */

  if (!mqtt.connected()) {
    Serial.println("---Reconnect MQTT ---");
    MQTT_reconnect();
  }
  mqtt.loop();

  Read_PM();
  Read_DHT();
  //Read_BMP();
  lcd_Show_16_2(pm1, pm2_5, dht_temp, dht_hum);
  Serial.println("=================");
  send_data();
  Serial.println("=================");
}

void Setup_WM() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;

  /*
  for (int i = 5; i > -1; i--) {
    delay(1000);
    lcd.print("Press For Res WM");
    int But_S = digitalRead(Pull_But);
    Serial.print("Time out in : ");
    lcd.print("Timeout in : ");
    Serial.println(i);
    lcd.setCursor(13, 1);
    lcd.print(i);
    Serial.print("Button Status : ");
    Serial.println(But_S);
    if (But_S == HIGH) {
      wm.resetSettings();
      lcd.setCursor(0, 1);
      lcd.print("Reset WM Waiting");
      break;
    }
    Serial.println("=======");
  }
  */

  bool res;
  res = wm.autoConnect("Project_2_64");

  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}

/*
void Setup_BMP() {
  Wire.begin();
  if (!bmp180.begin())
  {
    Serial.println("begin() failed. check your BMP180 Interface and I2C Address.");
    while (1);
  }
  bmp180.resetToDefaults();
  bmp180.setSamplingMode(BMP180MI::MODE_UHR);
}
*/

void Read_PM() {
  int index = 0;
  char value;
  char previousValue;

  while (mySerial.available()) {
    value = mySerial.read();
    if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4d)) {
      Serial.println("Cannot find the data header.");
      break;
    }

    if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14) {
      previousValue = value;
    }
    else if (index == 5) {
      pm1 = 256 * previousValue + value;
      Serial.print("\"pm1\": ");
      Serial.print(pm1);
      Serial.print(" ug/m3");
      Serial.print(", ");
    }
    else if (index == 7) {
      pm2_5 = 256 * previousValue + value;
      Serial.print("\"pm2_5\": ");
      Serial.print(pm2_5);
      Serial.print(" ug/m3");
      Serial.print(", ");
    }
    else if (index == 9) {
      pm10 = 256 * previousValue + value;
      Serial.print("\"pm10\": ");
      Serial.print(pm10);
      Serial.print(" ug/m3");
    } else if (index > 15) {
      break;
    }
    index++;
  }
  while (mySerial.available()) mySerial.read();
  Serial.println(" ");
  delay(1000);
}

void Read_DHT() {
  dht_hum = dht.readHumidity();
  dht_temp = dht.readTemperature();

  if (isnan(dht_hum) || isnan(dht_temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print("Hum = ");
  Serial.print(dht_hum);
  Serial.print("%");
  Serial.print("      ");
  Serial.print("Temp = ");
  Serial.print(dht_temp);
  Serial.println("C");
}

/*
void Read_BMP() {
  if (!bmp180.measureTemperature())
  {
    Serial.println("could not start temperature measurement, is a measurement already running?");
    return;
  }

  do
  {
    delay(100);
  } while (!bmp180.hasValue());

  bmp_temp = bmp180.getTemperature();

  Serial.print("Temperature: ");
  Serial.print(bmp_temp);
  Serial.println(" degC");

  if (!bmp180.measurePressure())
  {
    Serial.println("could not start perssure measurement, is a measurement already running?");
    return;
  }

  do
  {
    delay(100);
  } while (!bmp180.hasValue());

  bmp_press = bmp180.getPressure();

  Serial.print("Pressure: ");
  Serial.print(bmp_press);
  Serial.println(" Pa");
}
*/

void save_data() {
  doc["deviceid"] = deviceid;
  doc["pm1"] = pm1;
  doc["pm2_5"] = pm2_5;
  doc["pm10"] = pm10;
  doc["dht_hum"] = dht_hum;
  doc["dht_temp"] = dht_temp;
  //doc["bmp_temp"] = bmp_temp;
  //doc["bmp_press"] = bmp_press;
}

void send_data() {
  /* json flie */
  for (int i = 5; i > -1; i--) {
    delay(1000);
    Serial.println("Countdown" + String(i));
    Serial.println("Check_time : " + String(check_time));
    check_time++;
  }
  if (check_time >= 15) {
    save_data();
    /* MQTT Send to mqtt*/
    MQTT_senddata();
    /* Reset */
    check_time = 0;
  }
}
