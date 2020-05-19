#include <WiFi101.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LedControl.h>
#include <Servo.h>


//////// SENSORS ///////
#define MOISTURE_SENSOR_PIN  A0
#define TEMP_SENSOR_PIN  A1
#define LIGHT_SENSOR_PIN  A2


//////// ACTUATORS ////////

// WATER PUMP
#define WATER_PUMP_PIN  12
Servo servo;

// GREEN LED
#define LED_PIN_GREEN  14

// LED 8X8
#define DIN 8  // DATA IN FOR LED MATRIX
#define CS 7  // CHIP SELECT PIN
#define CLK 6 // PIN FOR CLOCK SPEED
LedControl ledControl = LedControl(DIN,CLK,CS,0);
byte smile[8]   = {0x3C,0x42,0xA5,0x81,0xA5,0x99,0x42,0x3C};
byte neutral[8] = {0x3C,0x42,0xA5,0x81,0xBD,0x81,0x42,0x3C};
byte sad[8]     = {0x3C,0x42,0xA5,0x81,0x99,0xA5,0x42,0x3C};
byte error[8]   = {0x81,0x42,0x24,0x18,0x18,0x24,0x42,0x81};
byte ok[8]      = {0x0,0x0,0x1,0x2,0x44,0x28,0x10,0x0};


//////// NETWORK CONFIG ////////
char ssid[] = "XXX";   // netwerk SSID
char pass[] = "XXX";     // netwerk w8w
int status = WL_IDLE_STATUS;      // connectie status
WiFiClient client;

//////// MQTT CONFIG ////////
char serverIp[] = "XXX";
int serverPort = 1883;
char topicSensors[] = "plantmonitor/sensors";
char topicActuatorLight[] = "plantmonitor/actuators/light";
char topicActuatorEmotion[] = "plantmonitor/actuators/emotion";
char topicActuatorWaterPump[] = "plantmonitor/actuators/waterpump";
char clientId[] = "plantMonitorArduino";
PubSubClient pubSubClient;


//////// GLOBAL VARIABLES ////////
int ledStatus = 0;
char jsonBody[50] = "";
bool errorHappened = false;
/////// FUNCTIONS //////////

float calculateTempInCelcius(int sensorValue) {
  return ((sensorValue * (3300/1024)) - 500) / 10;
}

void printByte(byte character [])
{
  int i = 0;
  for(i=0;i<8;i++)
  {
    ledControl.setRow(0,i,character[i]);
  }
}

// Wanneer een bericht binnenkomt via de client wordt deze functie getriggerd.
// Meer info kan gevonden worden op https://pubsubclient.knolleary.net/api.html#callback
void callBackFunction(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received message");
  Serial.print(topic);
  Serial.print("\n");
  if (strcmp(topic, topicActuatorLight) == 0) {
    if (payload[0] == '0') ledStatus = 0;
    if (payload[0] == '1') ledStatus = 1;
    digitalWrite(LED_PIN_GREEN, ledStatus);
  }

  if (strcmp(topic, topicActuatorEmotion) == 0 ) {
    if (payload[0] == '0' && payload[1] == '0') printByte(sad);
    if (payload[0] == '1' && payload[1] == '0') printByte(neutral);
    if (payload[0] == '1' && payload[1] == '1') printByte(smile);
  }

  if (strcmp(topic, topicActuatorWaterPump) == 0) {
    if (payload[0] == '1') servo.write(90);
    delay(5000);
    servo.write(0);
  }
}

void blinkLed(int port) {
  digitalWrite(LED_PIN_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN_GREEN, LOW);
  delay(1000);
}

void setup() {
  // nodig om de seriële monitor op te starten met een baud-rate
  Serial.begin(9600);


  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  pinMode(MOISTURE_SENSOR_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);

  ledControl.shutdown(0,false);
  ledControl.setIntensity(0,10);
  ledControl.clearDisplay(0);

  servo.attach(WATER_PUMP_PIN);

  // kijken of er wel een WiFi shield aanwezig is op de Arduino
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    printByte(error);
    while (true);       // don't continue
  }

  // nu gaan we connecteren tot het internet en blijven dit proberen zolang we geen connectie kunnen maken.
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }

  printWiFiStatus();

  // maken gebruik van een library PubSubClient om met de MQTT server te connecteren
  pubSubClient.setClient(client);               // zal luisteren naar binnenkomende berichten van topics waarop we subscriben
  pubSubClient.setServer(serverIp,serverPort);  // zal berichten publiceren naar een topic
  pubSubClient.setCallback(callBackFunction);   // deze callbackfunctie zal afgaan wanneer we een bericht ontvangen.

  // Checken of de servo werkt

  servo.write(0);
  delay(1000);
  servo.write(90);
  delay(1000);
  servo.write(0);
  
  // laten de led 3X blinken, zodat je weet dat de setup is gelukt
  blinkLed(LED_PIN_GREEN);
  blinkLed(LED_PIN_GREEN);
  blinkLed(LED_PIN_GREEN);
  printByte(ok);
  Serial.print("Plantmonitor is ready!\n");
}

void loop() {
  // lezen de temperatuur sensor
  int tempSensorValue = analogRead(TEMP_SENSOR_PIN);
  int moisterSensorValue = analogRead(MOISTURE_SENSOR_PIN);
  int lightSensorValue = analogRead(LIGHT_SENSOR_PIN);
  sprintf(jsonBody, "{\"temp\":%2.1f,\"moisture\":%1d,\"light\":%1d}", calculateTempInCelcius(tempSensorValue), moisterSensorValue, lightSensorValue);

  Serial.print("Reading value: ");
  Serial.print(jsonBody);
  Serial.print("\n");

   if (!pubSubClient.connected()) {
      if (!pubSubClient.connect(clientId)) {
          Serial.print("Connection to MQTT failed\n");
          printByte(error);
          errorHappened = true;
       }
       pubSubClient.subscribe(topicActuatorLight);
       pubSubClient.subscribe(topicActuatorEmotion);
       pubSubClient.subscribe(topicActuatorWaterPump);

    }

    if(pubSubClient.connected()) {
       Serial.print("Trying to publish value: ");
       Serial.print(jsonBody);
       Serial.print("\n");
       if (!pubSubClient.publish(topicSensors, jsonBody)) {
          Serial.print("Cannot publish value\n");
          printByte(error);
          errorHappened = true;
       }
    }


    for (int i = 0; i < 100; i++) {
      delay(10);
      pubSubClient.loop();
    }

    if (errorHappened) {
        errorHappened = false;
        printByte(ok);
     }

}


void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
