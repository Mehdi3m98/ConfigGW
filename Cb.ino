/*
Date:1402/04/05
Author: Mohammadmehdi Momen
ver:1.0
This code is written to configure the GW through the serial port.
*/



#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LoRa.h>
#include <LittleFS.h>

#define Di1 2
#define Di2 0
#define Do1 4
#define Do2 5
#define AnPin1 A0

//===========client config========

String clientId ;
WiFiClient espClient;
PubSubClient client(espClient);

//===========config===============

String unicodeString;
String input;
const char* connection_mod;
const char* ssid;
const char* password;
const char* mqtt_server;
unsigned long mqtt_port;
const char* mqtt_username;
const char* mqtt_password;
unsigned long Frequency;
unsigned long SignalBandwidth;
unsigned long SpreadingFactor;
unsigned long SyncWord;
unsigned long CodingRate4;
unsigned long TxPower;
unsigned long Gain;

//========LORA============
const int csPin = 15;          // LoRa radio chip select
const int resetPin = -1;       // LoRa radio reset
const int irqPin = 10;         // change for your board; must be a hardware interrupt pin

//convertByteToString

byte hexToByte(const char* hexString) {
  byte byteValue = 0;
  for (int i = 0; i < 2; i++) {
    char c = hexString[i];
    byteValue <<= 4;
    if (c >= '0' && c <= '9') {
      byteValue |= c - '0';
    } else if (c >= 'A' && c <= 'F') {
      byteValue |= c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
      byteValue |= c - 'a' + 10;
    }
  }
  return byteValue;
}

byte* hexStringToByteArray(String hexString) {
  int hexStringLength = hexString.length();
  int byteDataLength = hexStringLength / 2;
  byte* byteData = new byte[byteDataLength];
  for (int i = 0, j = 0; i < hexStringLength; i += 2, j++) {
    byteData[j] = hexToByte(hexString.substring(i, i + 2).c_str());
  }
  return byteData;
}

//=========================
//=========================

void serialcommand() {

   if (Serial.available()) {
        while (!Serial.available());
    String input = Serial.readString();
    byte* byteData = hexStringToByteArray(input);
    int byteDataLength = input.length() / 2;

    // Convert byte array to Unicode string
    String unicodeString;
    for (int i = 0; i < byteDataLength; i++) {
      unicodeString += (char)byteData[i];
    }
    // Print the Unicode string
    // Serial.println(unicodeString);
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, unicodeString);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    const char* connection_mod = doc["cn_m"]; // "pin_status"

    if (strcmp(connection_mod, "config") == 0) {
      // Handle config mode
      // Open the file in write mode
      File file = LittleFS.open("/inputt.txt", "w");
      if (!file) {
        Serial.println("Failed to open file for writing");
        return;
      }
      file.println(unicodeString);      
      // Close the file
      file.close();
        delay(10000);
      Serial.println("New Config Received!");
        Serial.flush();
        ESP.restart();

    }
    else if (strcmp(connection_mod, "pin_status") == 0) {
      // Handle pin status mode
      PIN_STATUS();
    }
  }
} 

//=========================
//read config and start

void setup_CONFIG() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  File file = LittleFS.open("/inputt.txt", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  // Read text from file
  String text = file.readString();
  Serial.println(text);
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, text);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  ssid = doc["ssid"];
  password = doc["pass"];
  mqtt_server = doc["mq_ser"];
  mqtt_port = doc["mq_po"];
  mqtt_username = doc["mq_un"];
  mqtt_password = doc["mq_pass"];
  clientId = doc["ci"].as<String>();    
  Frequency = doc["fre"];
  SignalBandwidth = doc["SiBa"];
  SpreadingFactor = doc["SpFa"];
  SyncWord = doc["SyWo"];
  CodingRate4 = doc["CoR4"];
  TxPower = doc["TxP"];
  Gain = doc["Ga"];

  file.close();
        // ESP.restart();
  //Setup LoRa/MQTT/WIFI

  setup_wifi();
  // setup_lora();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  delay(1000);
}

//=========================

void setup_wifi() {

  delay(2000);

  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    if (Serial.available() > 0) {
      delay(1000);
    serialcommand();

    }
  }
  Serial.println("ssid:");
  Serial.print(ssid);
  Serial.println("");
  Serial.println("Wi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Mac address: ");
  Serial.println(WiFi.macAddress());
}

//=========================

void setup_lora() {

  LoRa.setPins(csPin, resetPin, irqPin);
  

  if (!LoRa.begin(Frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSyncWord(SyncWord);
  LoRa.setCodingRate4(CodingRate4);
  LoRa.setTxPower(TxPower);
  LoRa.setGain(Gain);

  Serial.println("LoRa initialization successful!");
}

//=========================

void PIN_STATUS(){

 StaticJsonDocument<200> doc;

  doc["cn_m"] = "pin_status";

  JsonObject digital_in = doc.createNestedObject("digital_in");
  digital_in["di1"] = digitalRead(Di1);
  digital_in["di2"] = digitalRead(Di2);

  JsonObject digital_out = doc.createNestedObject("digital_out");
  digital_out["do1"] = digitalRead(Do1);
  digital_out["do2"] = digitalRead(Do2);
  doc["analog_in"] = analogRead(AnPin1);
  char PinStatus[512];
  serializeJson(doc, PinStatus);

  Serial.print(PinStatus);
  delay(100);
}

//=========================
//=========================

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char message[length+1];
  int i=0;
  for ( i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message[i]=(char)payload[i];
  }
  Serial.println();
}

//=========================

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("MQTT connection...");

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("CONNECT", clientId.c_str());
    } else if (Serial.available() > 0) {
      delay(1000);
      serialcommand(); // for config
    } else {
      Serial.print("failed,try again in 2 seconds");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(4000);
    }
  }
}

//=========================
//=========================
//=========================

void setup() {

Serial.begin(115200); 
Serial.setTimeout(1000);
Serial.setRxBufferSize(1024);

setup_CONFIG();

}

//=========================

void loop() {
   
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  serialcommand();

}

//=========================
//=========================
//=========================
//=========================