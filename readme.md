# IOT project guide

Het doel van het project is om het welzijn van een plant te monitoren, alsook dat eigenaars kunnen praten tegen hun plant, waarop de plant een antwoord zal geven.

## Benodigdheden project

- Arduino MKR1000

## Setup Mosquitto server
Voor het TCP/IP verkeer tussen de plant (Arduino= plants brain) en de eigenaar (web app) wordt het MQTT protocol gebruikt. MQTT gebruikt het publish/subscribe model om bi-derectionele communicatie tusen systemen toe te laten. Het voordeel van MQTT is dat het lightweight (binary messages) is, waardoor het geschikt is voor IOT waar geheugen optimalistie belangrijk is.


### Lokale setup via Docker host
Via docker kan heel eenvoudig een [Mosquitto](https://hub.docker.com/_/eclipse-mosquitto) server die het MQTT protocol ondersteunt opgezet worden.

```
  docker pull eclipse-mosquitto
```

Run vervolgens volgend docker commondo, maar zie dat de volumes zijn aangemaakt op de docker host. (pas zeker de paden aan!) De mosquitto.conf file dien je wel nog te [downloaden](https://github.com/eclipse/mosquitto/blob/master/mosquitto.conf) en te kopiëren naar de folder ``` ~/project/mosquitto/config ```.

```
docker run -it -p 1883:1883 -p 9001:9001 --name mqtt \
  -v mosquitto.conf:/Users/michael/Documents/KDG/IOT2/project/mosquitto/config/mosquitto.conf \
  -v /Users/michael/Documents/KDG/IOT2/project/mosquitto/data \
  -v /Users/michael/Documents/KDG/IOT2/project/mosquitto/log eclipse-mosquitto
```

Een simpele helloworld kan getest worden door twee terminals te openen waar we een subscriber zal luisteren naar een topic. Mockito heeft zelf een client en publisher CMD tool aan boord. Nu gezien we de setup in een docker container draaien en we deze CMD's niet vanop de host kunnen raadplegen dienen we twee interactieve terminal te open naar de docker instance genaamd ```mqtt```.
```
# Terminal 1
docker exec -ti mqtt "sh"
> mosquitto_sub -t test/sensor/temp
```

```
# Terminal 2
docker exec -ti mqtt "sh"
> mosquitto_pub -t test/sensor/temp -m 20
```

Als alles goed gaat krijg je in terminal 1 nu de output 20 te zien!

Tot slot nog een laatste check dat onze host zeker luistert naar de poort 1883:

```
nestat -an | grep -i 1883
> tcp46      0      0  *.1883                 *.*            LISTEN  
```
## Communicatie tussen de Arduino and de webapplicatie over MQTT
Het lezen van  MQTT berichten door een webapplicatie gebeurt veelal met websockets. Om dit mogelijk te maken dienen we nog extra config toe te voegen aan de ```mosquitto.conf ```file.

```
listener           8080
protocol           websockets
```
Vervolgens dienen we de docker instance te herstarten.
```
docker restart mqtt
```

### Arduino client code om te connecteren met de MQTT server

Hiervoor maken we gebruik van drie libraries:
- Met de [WiFi101](https://www.arduino.cc/en/Reference/WiFi101) library maken we connectie met het internet. Deze library is afhankelijk van het type WiFi shield je hebt op je Arduino.
- De [WiFiClient](https://www.arduino.cc/en/Reference/WiFiClient) laat ons toe over de WiFi te connecteren naar een server.
- Via de [PubSubClient](https://pubsubclient.knolleary.net/api.html) kunnen we zowel luisteren als berichten publiceren op een MQTT server. Deze library zorgt ervoor dat we geen low level TCP/IP pakketten zelf moeten sturen om te communiceren met een MQTT server.

De code en de uitleg kan teruggevonden worden in de file ```arduino/plantmonitor/plantmonitor.ino ```

mosquitto_pub -t PlantMonitor/sensor/light -m "1"



### Angular client code om te connecteren met de MQTT server
