#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "credentials.h"
#include "parameters.h"
#include "profile.h"
#include "cert.h"

/*** Define Board IO Pins ***/
#define mysensor 35
#define LDR 34
#define SPK 2
#define DualRed 18
#define DualGreen 19
#define Buzzer 5
#define SPDT 25
#define PB 23

/*** Define Expansion IO Pins ***/
#define RELAY 4

/*** Thresholds ***/
const float kHumidityLowThreshold = 30.0f;
const float kHumidityHighThreshold = 70.0f;
const float kLightBrightThreshold = 110.0f;
const float kLightDimThreshold = 70.0f;

/*** OLED setup ***/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*** Buzzer tone setup ***/
const int kSpeakerChannel = 0;
const int kSpeakerFrequency = 2000;
const int kSpeakerResolution = 8;

/*** Global variables ***/
float LDRsensor;
float sensor;
float humidity = 0.0f;
float lightLevel = 0.0f;
long timer;
bool currPBVal, oldPBVal;                       // States of PB (alarm silence)
bool alarmSilenced = false;
bool oledReady = false;
char publish_topic[100];
char subscribe_topic[100];

/*** WIFI and MQTT Client setup ***/
#if defined(HIVEMQS) || defined(MOSQUITTOS)     // Read 'profile.h' on #ifdef
WiFiClientSecure espClient;                     // Creates a client with secure TCP (SSL) connection via WIFI
#else
WiFiClient espClient;                           // Creates a client with TCP connection via WIFI
#endif
PubSubClient client(espClient);                 // Enable the client to connecf with MQTT

void setup() {
  /*** GPIO setup ***/
  pinMode(LDR, INPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(mysensor, INPUT);
  pinMode(DualRed, OUTPUT);
  pinMode(DualGreen, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(SPDT, INPUT_PULLUP);
  pinMode(PB, INPUT_PULLUP);
  pinMode(SPK, OUTPUT);

  /*** Initialise ***/
  oldPBVal = digitalRead(PB);            // PB state
  digitalWrite(RELAY, HIGH);             // RELAY pin HIGH to turn off
  digitalWrite(DualRed, LOW);
  digitalWrite(DualGreen, HIGH);
  digitalWrite(Buzzer, LOW);
  timer = millis();                      // timer
  set_topics();                          // setup MQTT topics

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
  } else {
    oledReady = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("IoT monitor boot");
    display.display();
  }

  /*** Setup Serial Communication ***/
  Serial.begin(115200);
  while (!Serial) delay(1);
  /*** Setup WIFI & MQTT Communication ***/
  #if defined(Publish) || defined(Subscribe)
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    client.setKeepAlive(keepalive);
    #ifdef HIVEMQS
      espClient.setCACert(root_ca1);       // Read 'cert.h' on MQTTS; Public Key of HIVEMQ
    #endif
    #ifdef MOSQUITTOS
      espClient.setCACert(root_ca2);       // Read 'cert.h' on MQTTS; Public Key of MOSQUITTO
    #endif
    mqttconnect();
  #endif
}

void loop() {
  if (millis() > timer) {                   // check timer to start publishing cycle
    /*** read LDR ***/
    sensor = analogRead(mysensor);          // Read Va binary code
    sensor = (sensor + 100) * 2048 / 1988;  // 2-pt compensation
    Serial.print("Sense binary:");
    Serial.print((short)sensor);
    sensor = sensor * 0.0008;               // Convert binary to voltage by x3.3/4095 ( = x0.0008)
    Serial.print(" / Sense Voltage Va (V):");
    Serial.print(sensor);
    humidity = ((sensor / 3.3) - 0.16) / 0.0062;
    Serial.print(" / Humidity (%RH): ");
    Serial.println(humidity);

    LDRsensor = analogRead(LDR);            // Read Va binary code
    LDRsensor = (LDRsensor + 100) * 2048 / 1988;  // 2-pt compensation
    Serial.print("Sense binary:");
    Serial.print((short)LDRsensor);
    LDRsensor = LDRsensor * 0.0008;         // Convert binary to voltage by x3.3/4095 ( = x0.0008)
    Serial.print(" / Sense Voltage Va (V):");
    Serial.print(LDRsensor);
    lightLevel = 200 / LDRsensor;

    if (lightLevel <= kLightDimThreshold) {
      Serial.println("  / light is dim");
    } else if (lightLevel >= kLightBrightThreshold) {
      Serial.println("  / light is bright");
    } else {
      Serial.println("  / light is normal");
    }
    #if defined(Publish)
      /*** JSON Object ***/
      DynamicJsonDocument doc(256);                         // JSON Object format: {"key1":value1,"key2":value2,...}
      doc["v"] = (short) digitalRead(SPDT);                      // 1st property eg. sense value
      doc["r"] = (short)! digitalRead(RELAY);
      doc["h"] = (short)humidity;
      doc["l"] = (short)lightLevel;
      char mqtt_message[256];
      serializeJson(doc, mqtt_message);

      /*** MQTT Publish ***/
      client.publish(publish_topic, mqtt_message, false);
      Serial.println("Published Topic: " + String(publish_topic));
      Serial.println("Published Message: " + String(mqtt_message));
    #endif
    /*** Set timer for next Transmit; non-blocking ***/
    timer = millis() + max(mintxinterval, 10000);
  }         //reset timer; end of publishing cycle
  /*** client.loop allow the client to process incoming subscribed messages thru callback() ***/
  #if defined(Publish) || defined(Subscribe)
    if (!client.loop()) mqttconnect();
  #endif
  bool highMode = digitalRead(SPDT) == HIGH; // HIGH = high-threshold mode, LOW = low-threshold mode
  bool humidityAlarm = highMode ? (humidity > kHumidityHighThreshold) : (humidity < kHumidityLowThreshold);
  bool isDim = lightLevel <= kLightDimThreshold;
  bool isBright = lightLevel >= kLightBrightThreshold;
  #if !defined(Subscribe)
    if (isDim) {
      digitalWrite(RELAY, LOW);
    } else {
      digitalWrite(RELAY, HIGH);
    }
  #endif
  if (!humidityAlarm) {
    alarmSilenced = false;
  }
  if (pbPressedChk()) {
    alarmSilenced = true;
  }
  if (humidityAlarm) {
    digitalWrite(DualGreen, LOW);
    digitalWrite(DualRed, HIGH);
    digitalWrite(Buzzer, alarmSilenced ? LOW : HIGH);
    if (alarmSilenced) {
      ledcWriteTone(kSpeakerChannel, 0);
    } else {
      ledcWriteTone(kSpeakerChannel, kSpeakerFrequency);
    }
  } else {
    digitalWrite(DualGreen, HIGH);
    digitalWrite(DualRed, LOW);
    digitalWrite(Buzzer, LOW);
    ledcWriteTone(kSpeakerChannel, 0);
  }
  if (oledReady) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Humidity: ");
    display.print(humidity, 1);
    display.println("%");
    display.print("Light: ");
    display.print(lightLevel, 1);
    if (isDim) {
      display.println(" (dim)");
    } else if (isBright) {
      display.println(" (bright)");
    } else {
      display.println(" (normal)");
    }
    display.print("Mode: ");
    display.println(highMode ? "High" : "Low");
    display.print("Relay: ");
    display.println(digitalRead(RELAY) == LOW ? "On" : "Off");
    display.print("Alarm: ");
    display.println(humidityAlarm ? (alarmSilenced ? "Silenced" : "Active") : "Normal");
    display.display();
  }
}
/*** MQTT Call back Method ***/
void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage += (char)payload[i];
  Serial.println("Message arrived [" + String(topic) + "]" + incommingMessage);
  if (String(topic).equalsIgnoreCase(subscribe_topic)) {                   //check if incoming topic == subscribe_topic
    if (incommingMessage.equalsIgnoreCase("ON"))
      { digitalWrite(RELAY, LOW); }
    else
      { digitalWrite(RELAY, HIGH); }
  }
}
/*** Connect to WiFi ***/
void setup_wifi() {
  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("w");
  }
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
}
void mqttconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientid, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      #if defined(Subscribe)
        client.subscribe(subscribe_topic, QoS);        // subscribe topic
      #endif
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");        // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
/*** Topics setup ***/
void set_topics() {
  sprintf(publish_topic, "%s/%s/%s", appname, clientid, device);
  sprintf(subscribe_topic, "DL/%s/%s/%s", appname, clientid, device);
}
/*** Alarm silence button ***/
bool pbPressedChk() {                       // returns true if PB is pressed, false if not pressed
  currPBVal = digitalRead(PB);
  if (currPBVal != oldPBVal) {              // if there is transition
    oldPBVal = currPBVal;
    if (currPBVal == LOW) {                 // if transition is HI to LO
      return true;
    }
  }
  return false;
}