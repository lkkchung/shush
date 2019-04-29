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

WiFiSSLClient net;
MqttClient mqtt(net);

int totalSteps = 200;
int currentStep = 100;

int lastValue = 0;

String testTopic = "shush/" + DEVICE_ID + "/test";
String soundTopic = "shush/" + DEVICE_ID + "/sound";

// Publish every 10 seconds for the workshop. Real world apps need this data every 5 or 10 minutes.
unsigned long publishInterval = 0.5 * 1000;
unsigned long lastMillis = 0;


void setup() {
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  resetEDPins(); //Set step, direction, microstep and enable pins to default states
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  Serial.println("Connecting WiFi");
  connectWiFi();

  for (i = 0; i < peaksLength; i++){
    peaks[i] = 0;
  }
  // digitalWrite(EN, HIGH);
}

// the loop routine runs over and over again forever:
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // if (!mqtt.connected()) {
  //   connectMQTT();
  // }
  
  // poll for new MQTT messages and send keep alives
  // mqtt.poll();

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

  // int newValue = abs(512 - sensorValue);
  // print out the value you read:
  Serial.println(newValue);
  // delay(500);        // delay in between reads for stability


  // if (millis() - lastMillis > publishInterval) {
  //   lastMillis = millis();

  //   Serial.print("Sound level: ");
  //   Serial.println(newValue);
    
  //   mqtt.beginMessage(testTopic);
  //   mqtt.print("Hello, world!"); 
  //   mqtt.endMessage();

  //   mqtt.beginMessage(soundTopic);
  //   mqtt.print(newValue); 
  //   mqtt.endMessage();
  // }  

  if(lastValue > newValue && currentStep < totalSteps){
    digitalWrite(dir, LOW);
    digitalWrite(stp, HIGH); //Trigger one step
    currentStep++;
    Serial.println("forward step");
  }

  if(lastValue < newValue && currentStep >= 0){
    digitalWrite(dir, HIGH);
    digitalWrite(stp, HIGH); //Trigger one step
    currentStep--;
    Serial.println("back step");
  }

  delay(1);
  digitalWrite(stp, LOW); //Pull step pin low so it can be triggered again
  resetEDPins();
  lastValue = newValue;
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


int loudness() {
   unsigned long startMillis= millis();                   // Start of sample window
   float peakToPeak = 0;                                  // peak-to-peak level
 
   unsigned int signalMax = 0;                            //minimum value
   unsigned int signalMin = 1024;                         //maximum value
 
                                                          // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(A1);                             //get reading from microphone
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

//Reset Easy Driver pins to default states
void resetEDPins()
{
  digitalWrite(stp, LOW);
  digitalWrite(dir, LOW);
  digitalWrite(MS1, HIGH);
  digitalWrite(MS2, HIGH);
  // digitalWrite(EN, HIGH);
}