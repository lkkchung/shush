#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

#include "config.h"

WiFiSSLClient net;
MqttClient mqtt(net);

String testTopic = "shush/" + DEVICE_ID + "/test";
String soundTopic = "shush/" + DEVICE_ID + "/sound";

// Publish every 10 seconds for the workshop. Real world apps need this data every 5 or 10 minutes.
unsigned long publishInterval = 0.5 * 1000;
unsigned long lastMillis = 0;


void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  Serial.println("Connecting WiFi");
  connectWiFi();
}

// the loop routine runs over and over again forever:
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqtt.connected()) {
    connectMQTT();
  }
  
  // poll for new MQTT messages and send keep alives
  mqtt.poll();

  // read the input on analog pin 0:
  int sensorValue = analogRead(A1);
  int newValue = abs(512 - sensorValue);
  // print out the value you read:
  Serial.println(newValue);
  // delay(500);        // delay in between reads for stability

  if (millis() - lastMillis > publishInterval) {
    lastMillis = millis();

    Serial.print("Sound level: ");
    Serial.println(newValue);
    
    mqtt.beginMessage(testTopic);
    mqtt.print("Hello, world!"); 
    mqtt.endMessage();

    mqtt.beginMessage(soundTopic);
    mqtt.print(newValue); 
    mqtt.endMessage();
  }  
}

void connectWiFi() {
  // Check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  Serial.print("WiFi firmware version ");
  Serial.println(WiFi.firmwareVersion());
  
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(WIFI_SSID);
  Serial.print(" ");

  while (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(3000);
  }

  Serial.println("Connected to WiFi");
  printWiFiStatus();

}

void connectMQTT() {
  Serial.print("Connecting MQTT...");
  mqtt.setId(DEVICE_ID);
  mqtt.setUsernamePassword(MQTT_USER, MQTT_PASSWORD);

  while (!mqtt.connect(MQTT_BROKER, MQTT_PORT)) {
    Serial.print(".");
    delay(5000);
  }

  Serial.println("connected.");
}

void printWiFiStatus() {
  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}
