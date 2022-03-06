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
