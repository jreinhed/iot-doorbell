/***************************************************
  LoLin code for a MQTT client / server controlled doorbell. 
 ****************************************************/
 
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "XXXXXX"
#define WLAN_PASS       "XXXXXX"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "192.168.10.39"
#define AIO_SERVERPORT  1883                   
#define AIO_USERNAME    "Doorbell_1"
#define AIO_KEY         "...your AIO key..."  // unused but kept for increased security needs

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'doorbell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish doorknob_1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/doorbell");

// Setup a feed called 'reply' for subscribing to changes.
Adafruit_MQTT_Subscribe Led_reply = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/reply");

/*************************** Sketch Code ************************************/

void MQTT_connect();

// variables
int buttonstate = 0;
const int button_pin = D1;
const int led_pin = D3;

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(button_pin, INPUT);
  pinMode(led_pin, OUTPUT);

  Serial.println(F("Doorbell MQTT started"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for reply feed.
  mqtt.subscribe(&Led_reply);
}

void loop() {
  // Ensure the connection to the MQTT server is alive
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;

  buttonstate = digitalRead(button_pin);
  if (buttonstate == HIGH) {   // The button is pressed
    if (!doorknob_1.publish("pling plong!")) {
      Serial.println(F("Publish failed"));
    } else {
      // waits for a reply from the Rpi client to "open the door" 
      while ((subscription = mqtt.readSubscription(9000))) {
        if (subscription == &Led_reply) {
          // Door is unlocked for 5 seconds
          digitalWrite(led_pin, HIGH);
          delay(5000);
          digitalWrite(led_pin, LOW);
        }
      }
    }
  }
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
