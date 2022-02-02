/* Libary */
#include <SoftwareSerial.h>
#include <DHT.h>
#include <Arduino.h>
#include <Wire.h>
#include <BMP180I2C.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

/* Define */
#define DHTPIN D1
#define DHTTYPE DHT11
#define BMP_I2C_ADD 0x77

/*  */
DHT dht(DHTPIN, DHTTYPE);
BMP180I2C bmp180(BMP_I2C_ADD);
SoftwareSerial mySerial(D2, D3); // RX, TX
WiFiClient espClient;
WiFiManager wm;
PubSubClient mqtt(espClient);

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
//Test server
const char* mqttServer = "kraiwich.thddns.net";
const int mqttPort = 4441;
const char* mqttUser = "kraiwich";
const char* mqttPassword = "kraiwich1234";
char mqtt_name_id[] = "";
const char* pubTopic = "kraiwich/project";

void setup() {
  Serial.begin(9600);
  while (!Serial) ;
  mySerial.begin(9600);
  Setup_WM();
  dht.begin();
  Setup_BMP();
  MQTT_connect();
  delay(1000);
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
  Read_BMP();
  Serial.println("=================");
  send_data();
  Serial.println("=================");
}

void Setup_WM() {
  WiFi.mode(WIFI_STA);

  bool res;
  res = wm.autoConnect("AutoConnectAP", "password");

  //wm.resetSettings();

  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}
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

void save_data() {
  doc["deviceid"] = deviceid;
  doc["pm1"] = pm1;
  doc["pm2_5"] = pm2_5;
  doc["pm10"] = pm10;
  doc["dht_hum"] = dht_hum;
  doc["dht_temp"] = dht_temp;
  doc["bmp_temp"] = bmp_temp;
  doc["bmp_press"] = bmp_press;
}

void send_data() {
  /* json flie */
  for (int i = 5; i > -1; i--) {
    delay(1000);
    Serial.println("Countdown" + String(i));
    Serial.println("Check_time : " + String(check_time));
    check_time++;
  }
  if (check_time >= 30) {
    save_data();
    /* MQTT Send to mqtt*/
    MQTT_senddata();
    /* Reset */
    check_time = 0;
  }
}

void MQTT_callback(char* topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void MQTT_connect() {
  Serial.print("Set mqtt server :");
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(MQTT_callback);
  Serial.println(" Done ");
  delay(100);
}

void MQTT_reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(mqtt_name_id, mqttUser, mqttPassword)) {
      Serial.println("-> MQTT mqtt connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println("-> try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void MQTT_senddata() {
  size_t n = serializeJson(doc, buffer);
  if (mqtt.connect(mqtt_name_id, mqttUser, mqttPassword)) {
    Serial.println("\nConnected MQTT: ");
    if (mqtt.publish(pubTopic, buffer , n) == true) {
      Serial.println("publish success");
    } else {
      Serial.println("publish Fail");
    }
  } else {
    Serial.println("Connect Fail MQTT");
  }
  Serial.println("====== JSON BODY =======");
  serializeJsonPretty(doc, Serial);
  Serial.println("======= JSON BODY END ======");
}
