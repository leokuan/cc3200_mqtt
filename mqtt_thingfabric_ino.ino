/*
PURPOSE: Continuously publish temperature sensor and accelerometer sensor values to the 2lemetry thingfabric platform,
  also, subscribe to a "message" topic to receive "ON/OFF" messages, used to enable/disable sending of data.
  
HOW TO USE
  1. Obtain a free developer account at 2lemetry.com, and log into your account
  2. Use the default thinkfabric project, or create a new one of your own.
  3. Obtain your credentials to connect your CC3200 device to your project
     a. Key (Username) - this is your username
     b. MD5 Secret - this is the MD5 hash for your password
  4. The project name is used as the first level in the topic to publish/subscribe
  5. Modify this sketch with your credentials and project name. These are the values:
     USER_ID, USER_PASSWORD, PUBLISH_TOPIC, and SUBSCRIBE_TOPIC
  6. Modify WIFI_SSID with your access point name, and WIFI_PWD with your WPA password 
  7. Install the mqtt library and TMP006 libraries as described below
  
INSTALLING THE LIBRARIES   
  1. This example is based on the arduino client for mqtt provided by Nick O'Leary
     at http://knolleary.net/arduino-client-for-mqtt/
     You should download this modified version from  
     http://energia.nu/wordpress/wp-content/uploads/2014/09/PubSubClient.zip
     The "PubSubClient" folder should be copied to the libraries folder in your sketchbook location.
     By default on Windows 7, this is:  c:\Users\<username>\Documents\Energia\libraries
     Of course you can change the sketchbook location using File-Preferences.
     You can read more about this mqtt client at http://energia.nu/blog/

  2. The current Energia release (0101E0013) does not contain a library for the TMP006 temperature sensor, so this
     example uses the Adafruit library from https://github.com/adafruit/Adafruit_TMP006
     Copy this TMP006 folder to your libraries folder as well. You will want to make one change to the
     "Adafruit_TMP006.h" file - simply comment out the #include <Adafruit_Sensor.h> line.
*/

#include <WiFi.h>
#include <Wire.h>
#include <BMA222.h>
#include <Adafruit_TMP006.h>
#include <PubSubClient.h>

// #include <SPI.h> //only required if using an MCU LaunchPad + CC3100 BoosterPack. Not needed for CC3200 LaunchPad
WiFiClient wclient;
BMA222 accelSensor; 
Adafruit_TMP006 tmp006(0x41);

//byte server[] = { 198, 41, 30, 241 }; //  Public MQTT Brokers: http://mqtt.org/wiki/doku.php/public_brokers

//byte server[] = {50, 19, 208, 210}; // 2lemetry mqtt broker (q.m2m.io)
char *server = "q.m2m.io";
//byte ip[]     = { 172, 16, 0, 100 };
byte ip[]     = { 0, 0, 0, 0 };

byte mac[6];  // MAC address for the device
String macAscii = "";

#define       WIFI_SSID         "wrap" // replace this with your SSID 
#define       WIFI_PWD          "gatordontplaynoshit"    // replace this with your WPA password

#define       CLIENT_ID          "CC3200_test"
// USER_ID is your Key (Username) from the project credentials page
#define       USER_ID            "01e51e1b-dbb6-4198-894b-7ef22ef332b5"
// USER_PASSWORD is your MD5 Secret from the project credentials page
#define       USER_PASSWORD      "041e21bb52ba5946f4620f46862045e0"
// The first level for PUBLISH_TOPIC and SUBSCRIBE_TOPICshould be your project name
// Note: the PUBLISH_TOPIC is appended with "_XXXXXXXXXXXX", the X's are your MAC address
#define       PUBLISH_TOPIC      "beno5ofcf1owrha/cc3200_test"
#define       SUBSCRIBE_TOPIC    "beno5ofcf1owrha/message"

#define   MAX_PUB_TOPIC_CHARS 256
String pubTopicString;  //used to store the PUBLISH_TOPIC with underscore and MAC address appended
int pubLen;
char pubTopic[MAX_PUB_TOPIC_CHARS];

boolean toPublishOrNotToPublish = true;

PubSubClient client(server, 1883, callback, wclient);

void callback(char* inTopic, byte* payload, unsigned int length){
// Handle callback here

    payload[length] = 0; //append null character to the byte array
    Serial.print("Payload received: "); Serial.println((char *)payload);
    if(strstr((char *)payload,"ON") != 0) {
      Serial.println("Received ON command");
      toPublishOrNotToPublish = true;
    }
    if(strstr((char *)payload,"OFF") != 0) {
      Serial.println("Received OFF command");
      toPublishOrNotToPublish = false;
    }
}

void setup()
{
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  Serial.println("Start WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  while(WiFi.localIP() == INADDR_NONE) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("");
  printWifiStatus();

// get MAC address
  WiFi.macAddress(mac);
  String macChars = "";
  macChars = String(mac[5],HEX);
  if (macChars.length() == 1) {macChars = "0" + macChars;}
  macAscii = macChars;
  
  macChars = String(mac[4],HEX);
  if (macChars.length() == 1) {macChars = "0" + macChars;}
  macAscii = macAscii + macChars;
  
  macChars = String(mac[3],HEX);
  if (macChars.length() == 1) {macChars = "0" + macChars;}
  macAscii = macAscii + macChars;

  macChars = String(mac[2],HEX);
  if (macChars.length() == 1) {macChars = "0" + macChars;}
  macAscii = macAscii + macChars;
 
  macChars = String(mac[1],HEX);
  if (macChars.length() == 1) {macChars = "0" + macChars;}
  macAscii = macAscii + macChars;

  macChars = String(mac[3],HEX);
  if (macChars.length() == 1) {macChars = "0" + macChars;}
  macAscii = macAscii + macChars;
  Serial.print("MAC address: ");
  Serial.println(macAscii);
  
  pubTopicString = PUBLISH_TOPIC; 
  pubTopicString = pubTopicString + "_" + macAscii;
  Serial.print("Publish topic: ");
  Serial.println(pubTopicString);
  pubLen = pubTopicString.length() + 1;
  pubTopicString.toCharArray(pubTopic, pubLen);
  
// initialize the accelerometer
  accelSensor.begin();
  uint8_t chipID = accelSensor.chipID();
  Serial.print("chipID: ");
  Serial.println(chipID);

// initialize the TMP006 sensor
  if (! tmp006.begin()) {
    Serial.println("No sensor found");
  }  

// connect to mqtt broker at 2lemetry (q.m2m.io) ip address is 50.19.208.120
  Serial.println("Connecting to 2lemetry thingfabric broker (q.m2m.io) "); 
  while (!client.connect(CLIENT_ID, USER_ID, USER_PASSWORD) ){
    Serial.print(".");
    delay(300);
  }
  Serial.println("Connected to 2lemetry thingfabric broker");
  
  if (  client.subscribe(SUBSCRIBE_TOPIC)) {
    Serial.print("Subscribe succeeded: ") ;
  }
  else {
    Serial.print("Subscribe failed: ");
  }
  Serial.println(SUBSCRIBE_TOPIC);
}

void loop() {
  // toPublishOrNotToPublish controls whether or not to publish data
  // from the thingfabric project, you can publish to <project>/message the value ON or OFF
  // see the callback function above to see the simple parsing of this subscribed message
  if (toPublishOrNotToPublish) {
    publishSensors();
  }
  
// If we got disconnected, try to re-connect
  if (! client.connected()) {
    if( client.connect(CLIENT_ID, USER_ID, USER_PASSWORD)) {
      Serial.println("Reconnected to broker");
      }
    else {
      Serial.println("Can't reconnect to broker");
    }
  }

// Need to call client.loop() to ensure we check for subscribed values
// because callback() isn't called unless we periodically call client.loop()
  client.loop();
  delay(10000);
  client.loop();
  delay(10000);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void publishSensors() {
#define  ACCEL_X  "\"Accel_X\":"
#define  ACCEL_Y  "\"Accel_Y\":"
#define  ACCEL_Z  "\"Accel_Z\":"
#define  TEMP  "\"Temp\":"
#define  MAX_JSON_CHARS  256

// read the input from the CC3200 on-board accelerometer
  int8_t xdata = accelSensor.readXData(); //  Serial.print("X: "); Serial.print(xdata);
  int8_t ydata = accelSensor.readYData(); //  Serial.print("Y: "); Serial.print(ydata);
  int8_t zdata = accelSensor.readZData(); //  Serial.print("Z: "); Serial.println(zdata);

// read the temperature from TMP006 sensor
//  //tmp006.wake();
  float objtC = tmp006.readObjTempC(); // Serial.print("Object Temperature: "); Serial.print(objt); Serial.println("*C");
  float objtF = (objtC * (9.0 / 5.0)) + 32.0;
  int8_t objtIntC = int(objtC);
  int8_t objtIntF = int(objtF);
//  float diet = tmp006.readDieTempC(); // Serial.print("Die Temperature: "); Serial.print(diet); Serial.println("*C");

  String jsonString;
  char jsonChars[MAX_JSON_CHARS];
  jsonString = "{";
  jsonString = jsonString + ACCEL_X; jsonString = jsonString + xdata; jsonString = jsonString + ",";
  jsonString = jsonString + ACCEL_Y; jsonString = jsonString + ydata; jsonString = jsonString + ",";
  jsonString = jsonString + ACCEL_Z; jsonString = jsonString + zdata; jsonString = jsonString + ",";
  jsonString = jsonString + TEMP; jsonString = jsonString + objtIntC; jsonString = jsonString + "}"; //use this if you want Celsius
 // jsonString = jsonString + TEMP; jsonString = jsonString + objtIntF; jsonString = jsonString + "}"; //use this if you want Fahrenheit
  jsonString.toCharArray(jsonChars, jsonString.length() + 1); 
  publishJsonString(pubTopic, jsonChars);
}

void publishJsonString(char *topic,char *json) {

  if (client.publish(topic, json)){
    Serial.print("Publish success: ");
    }
  else {
    Serial.print("Publish failed:  ");
    }
  Serial.println(json);
}

