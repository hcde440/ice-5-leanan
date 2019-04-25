/*  Leana Nakkour (Publisher)
 *  Partner & Subscriber: Melody Xu
 *  HCDE 440 - ICE #5
 *  April 25, 2019
 */

#include <ESP8266WiFi.h>    //Requisite Libraries . . .
#include "Wire.h"           //
#include <PubSubClient.h>   //
#include <ArduinoJson.h>    //
#include <Adafruit_MPL115A2.h>  // Barometric pressure sensor library
#include <Adafruit_Sensor.h>    // the generic Adafruit sensor library used with both sensors
#include <DHT.h>                // temperature and humidity sensor library
#include <DHT_U.h>              // unified DHT library

// Defines and Creates Sensor instances
#define DATA_PIN 12                   // pin connected to DH22 data line
DHT_Unified dht(DATA_PIN, DHT22);     // create DHT22 instance
Adafruit_MPL115A2 mpl115a2;           // create MPL115A2 instance

////// Wifi /////
#define wifi_ssid "University of Washington"  
#define wifi_password "" 

///// MQTT server /////
#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

WiFiClient espClient;                     //blah blah blah, espClient
PubSubClient mqtt(espClient);             //blah blah blah, tie PubSub (mqtt)

char mac[6];                              //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
char message[201];                        //201, as last character in the array is the NULL character, denoting the end of the array
unsigned long currentMillis = millis();
unsigned long timerOne, timerTwo, timerThree;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);                       //register the callback function
  timerOne = timerTwo = timerThree = millis();      // Sets each timer to current time

  mpl115a2.begin(); // initialize MPL115a2
  while(! Serial);
  dht.begin();      // initialize dht22

}

/////SETUP_WIFI/////
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}                                     //5C:CF:7F:F0:B0:C1 for example


/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("fromMarco/+"); // fromMarco (sensors) to Polo (display) we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  /// Conditional to refresh/reconnect the temperature sensor every 5 seconds
  if (millis() - timerOne > 5000) {
    sensors_event_t event;                // Initializes the event (inforamtion read by sensor)
    dht.temperature().getEvent(&event);   // getEvent gets the TEMP information read by the DH22 

    float temp = event.temperature;
    float fahrenheit = (temp * 1.8) + 32;     // Converts temp to fahrenheit
    
    dht.humidity().getEvent(&event);          // gets Humidity "event"
    float hum = event.relative_humidity;
    float pre = 0;                       // sets variable to hold pressure data
    pre = mpl115a2.getPressure();        // gets pressure data
      
    char str_temp[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    char str_hum[6]; //a temp array of size 6 to hold "XX.XX" + the terminating character
    char pressure[6];
    
    //take temp, format it into 5 char array with a decimal precision of 2, and store it in str_temp
    dtostrf(temp, 5, 2, str_temp);
    //ditto
    dtostrf(hum, 5, 2, str_hum);
    dtostrf(pre, 5, 2, pressure);

    // Publish message to the tem/hum topic 
    sprintf(message, "{\"Temperature\":\"%s\", \"Humidity\":\"%s\", \"Pressure\":\"%s\"}", str_temp, str_hum, pressure); //JSON format using {"XX":"XX"}
    mqtt.publish("fromMarco/tempHum", message);
    timerOne = millis();

  }
}


/////CALLBACK/////
//The callback is where we attach a listener to the incoming messages from the server.
//By subscribing to a specific channel or topic, we can listen to those topics we wish to hear.
//We place the callback in a separate tab so we can edit it easier . . . (will not appear in separate
//tab on github!)
/////

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  /////
  //We can use strcmp() -- string compare -- to check the incoming topic in case we need to do something
  //special based upon the incoming topic, like move a servo or turn on a light . . .
  //strcmp(firstString, secondString) == 0 <-- '0' means NO differences, they are ==
  /////

  if (strcmp(topic, "fromMarco/LBIL") == 0) {
    Serial.println("A message from Batman . . .");
  }

  else if (strcmp(topic, "fromMarco/tempHum") == 0) {
    Serial.println("Some weather info has arrived . . .");
  }

  else if (strcmp(topic, "fromMarco/switch") == 0) {
    Serial.println("The switch state is being reported . . .");
  }

  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}
