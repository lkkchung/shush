#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

#include "config.h"

#define stp 2
#define dir 3
#define MS1 4
#define MS2 5
#define EN  6

int state;
int i;

const int peaksLength = 9;
int peaks[peaksLength];

const int sampleWindow = 50;                              // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;

// WiFiSSLClient net;
WiFiClient net;
MqttClient mqtt(net);

int totalSteps = 200;
int pos = 0; //value of 0 to 2000
float stepDir = 1; //value of 1 or negative 1;


int lastValue = 0;

String testTopic = DEVICE_ID + "/test";
String soundTopic = DEVICE_ID + "/sound";
String ledTopic = DEVICE_ID + "/led";
String sensorTopic = DEVICE_ID + "/sensor";
String stepperTopic = DEVICE_ID + "/stepper";

// Publish every 10 seconds for the workshop. Real world apps need this data every 5 or 10 minutes.
unsigned long publishInterval = 2 * 1000;
unsigned long lastMillis = 0;


void setup() {
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  pinMode(7, OUTPUT);
  resetEDPins(); //Set step, direction, microstep and enable pins to default states
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  Serial.println("Connecting WiFi");
  connectWiFi();

  for (i = 0; i < peaksLength; i++){
    peaks[i] = 0;
  }
  // digitalWrite(EN, HIGH);
  // define function for incoming MQTT messages
  mqtt.onMessage(messageReceived);
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

  for (i = 1; i < peaksLength; i++){
    peaks[i-1] = peaks[i];
  }

  // read the input on analog pin 0:
  int sensorValue = analogRead(A1);

  peaks[peaksLength-1] = loudness();

  int newValue = 0;
  int smoothWeight = 0;

  for (i = 0; i < peaksLength; i++){
    int c = i;
    if(i >= peaksLength){
      c = abs(i - peaksLength);
    }
    newValue += peaks[i];
    smoothWeight += c;
  }

  newValue /= smoothWeight;

  lastValue --;
  if (lastValue > newValue) {
    newValue = lastValue;
  } else {
    lastValue = newValue;
  }

  // int newValue = abs(512 - sensorValue);
  // print out the value you read:


  int dest = newValue; //value of 12 to 197
  char user_input = Serial.read();
  pos = map(analogRead(A2), flexHigh, flexLow, 60, 180);

  int dist = dest - pos;

  if (dist > 0) {
    digitalWrite(dir, HIGH);
    // stepDir = 1;
  } else {
    digitalWrite(dir, LOW);
    // stepDir = -1;
  }

  int newDist = abs(dist);
  
  int spd = newDist;

  // if (spd <= 150){
  //   spd /= 3;    
  // } else {
  //   spd = 49;
  // }

  // Serial.print(" | ");
  // delay(500);        // delay in between reads for stability
  stepping(spd);

  if (millis() - lastMillis > publishInterval) {
    lastMillis = millis();

    Serial.print("Sound level: ");
    Serial.println(newValue);

    mqtt.beginMessage(soundTopic);
    mqtt.print(newValue); 
    mqtt.endMessage();

    mqtt.beginMessage(sensorTopic);
    mqtt.print(pos); 
    mqtt.endMessage();
  }  
  
  Serial.print(dest);
  Serial.print(" s|p ");

}

int loudness() {
   unsigned long startMillis= millis();                   // Start of sample window
   float peakToPeak = 0;                                  // peak-to-peak level
 
   unsigned int signalMax = 0;                            //minimum value
   unsigned int signalMin = 1024;                         //maximum value
 
                                                          // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(A1);                             //get reading from microphone
      // Serial.println(sample);
      if (sample < 1024)                                  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;                           // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;                           // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;                    // max - min = peak-peak amplitude
   return peakToPeak;

}

void stepping(int spd) {
  // for (int i = 0; i < 50 ; i++) {
  //   if (i < spd) {
  //     digitalWrite(2, HIGH);
  //   } else {
  //     digitalWrite(2, LOW);
  //     delay(1);
  //   }
  //   pos += stepDir;
  // }
  if(pos > 0 && pos < 170){
  digitalWrite(stp, HIGH);
  }
  Serial.println(pos);
}

//Reset Easy Driver pins to default states
void resetEDPins() {
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, HIGH);
  digitalWrite(MS2, HIGH);
  // digitalWrite(EN, HIGH);
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
  
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  int ssidChoice = 0;
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");

      int compVal;
      for (int j = 0; j < 2; j++){
        compVal = strcmp(WiFi.SSID(i), WIFI_SSID[j]);
        if (compVal == 0){
          Serial.print("Connecting to");
          Serial.println(WiFi.SSID(i));
          ssidChoice = j;
        }

      }
      delay(10);
    }
  }
  Serial.println("");

  Serial.print("Attempting to connect to SSID: ");
  Serial.print(WIFI_SSID[ssidChoice]);
  Serial.print(" ");

  while (WiFi.begin(WIFI_SSID[ssidChoice], WIFI_PASSWORD[ssidChoice]) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    digitalWrite(7, HIGH);
    delay(750);
    digitalWrite(7, LOW);
    delay(750);
    digitalWrite(7, HIGH);
    delay(750);
    digitalWrite(7, LOW);
    delay(750);
  }
  

  Serial.println("Connected to WiFi");
  printWiFiStatus();

}

void printWiFiStatus() {
  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void connectMQTT() {
  Serial.print("Connecting MQTT...");
  mqtt.setId(DEVICE_ID);
  mqtt.setUsernamePassword(MQTT_USER, MQTT_PASSWORD);

  while (!mqtt.connect(MQTT_BROKER, MQTT_PORT)) {
    Serial.print(".");
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqtt.connectError());
    digitalWrite(7, HIGH);
    delay(500);
    digitalWrite(7, LOW);
    delay(500);
    digitalWrite(7, HIGH);
    delay(500);
    digitalWrite(7, LOW);
    delay(1000);
  }

  Serial.println("connected.");

  mqtt.subscribe(ledTopic);
  Serial.println("connected to led topic.");  
  mqtt.subscribe(stepperTopic);
  Serial.println("connected to led topic.");
}

void messageReceived(int messageSize) {
  String topic = mqtt.messageTopic();
  String payload = mqtt.readString();
  Serial.println("incoming: " + topic + " - " + messageSize + " ");
  Serial.println(payload);
  if (payload == "ON") {
    // turn the LED on
    digitalWrite(7, HIGH);
  } else if (payload == "OFF") {
    // turn the LED off
    digitalWrite(7, LOW);    
  } else {
    analogWrite(7, payload.toInt());
  }
}