#include <WiFi101.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#define LED_PIN_GREEN  6
#define TEMP_PIN  A6
#define MOISTURE_PIN  A5
#define POWER_PIN_TEMP 13
#define POWER_PIN_MOISTURE 14

// netwerk config
char ssid[] = "XXX";           // netwerk SSID
char pass[] = "XXX";      // netwerk w8w
int status = WL_IDLE_STATUS;      // connectie status

// MQTT connectie config
char serverIp[] = "192.168.0.252";
int serverPort = 1883;
char topicSensorTemp[] = "PlantMonitor/sensor/temp";
char topicSensorLight[] = "PlantMonitor/sensor/light";
char clientId[] = "plantMonitorArduino";


WiFiClient client;
PubSubClient pubSubClient;

int ledStatus = 0;
char jsonBody[50] = "";

// Wanneer een bericht binnenkomt via de client wordt deze functie getriggerd.
// Meer info kan gevonden worden op https://pubsubclient.knolleary.net/api.html#callback
void callBackFunction(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received message");
  if (payload[0] == '0') ledStatus = 0;
  if (payload[0] == '1') ledStatus = 1;
  digitalWrite(LED_PIN_GREEN, ledStatus);
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
  pinMode(TEMP_PIN, INPUT);
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(POWER_PIN_TEMP, OUTPUT);
  pinMode(POWER_PIN_MOISTURE, OUTPUT);


  // kijken of er wel een WiFi shield aanwezig is op de Arduino
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
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

  // laten de led 3X blinken, zodat je weet dat de setup is gelukt
  blinkLed(LED_PIN_GREEN);
  blinkLed(LED_PIN_GREEN);
  blinkLed(LED_PIN_GREEN);
  Serial.print("Plantmonitor is ready!\n");

  // eerst even stroom op de potentiemeter zetten we de power aan, zodat de potentiemeter stroom krijgt.
  digitalWrite(POWER_PIN_MOISTURE, HIGH);
    digitalWrite(POWER_PIN_TEMP, HIGH);

}

void loop() {
  // lezen de temperatuur sensor
  int tempSensorValue = analogRead(TEMP_PIN);
  int moisterSensorValue = analogRead(MOISTURE_PIN);
  sprintf(jsonBody, "{\"temp\":%1d,\"moisture\":%1d,\"light\":10}",tempSensorValue, moisterSensorValue);

  Serial.print("Reading value: ");
  Serial.print(jsonBody);
  Serial.print("\n");

   if (!pubSubClient.connected()) {
      if (!pubSubClient.connect(clientId)) {
          Serial.print("Connection to MQTT failed\n");
       }
       pubSubClient.subscribe(topicSensorLight);
    }

    if(pubSubClient.connected()) {
       Serial.print("Trying to publish value: ");
       Serial.print(jsonBody);
       Serial.print("\n");
       if (!pubSubClient.publish(topicSensorTemp, jsonBody)) {
          Serial.print("Cannot publish value\n");
       }
    }

    for (int i = 0; i < 100; i++) {
      delay(10);
      pubSubClient.loop();
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
