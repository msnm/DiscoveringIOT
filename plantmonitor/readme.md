# IOT project guide

Het doel van het project is om het welzijn van een plant te monitoren en dit op een ludieke manier, als ook interactie mogelijk te maken tussen de fysieke plant en gebruiker via de app.

## Benodigdheden project

- Arduino MKR1000
- temperatuur sensor [TMP36 GZ](https://www.analog.com/media/en/technical-documentation/data-sheets/TMP35_36_37.pdf) (zou ook de TMP38 kunnen zijn, want moeilijk leesbaar, maar over de laatste vind ik geen datasheet)
- phototransistor + 1K Ohmse weerstand
- LED lampje + 560 Ohmse weerstand
- DIY moisture sensor + 1M Ohmse weerstand
- 8X8 Matrix LED paneel
- Servo motor

## Setup Mosquitto server
Voor het TCP/IP verkeer tussen de plant (Arduino= plants brain) en de eigenaar (web app) wordt het MQTT protocol gebruikt. MQTT gebruikt het publish/subscribe model om bi-derectionele communicatie tusen systemen toe te laten. Het voordeel van MQTT is dat het lightweight (binary messages) is, waardoor het geschikt is voor IOT waar geheugen optimalistie belangrijk is.


### Lokale setup via Docker host
Via docker kan heel eenvoudig een [Mosquitto](https://hub.docker.com/_/eclipse-mosquitto) server die het MQTT protocol ondersteunt opgezet worden.

```
  docker pull eclipse-mosquitto
```

Run vervolgens volgend docker commondo, maar zie dat de volumes zijn aangemaakt op de docker host. (pas zeker de paden aan!) De mosquitto.conf file dien je wel nog te [downloaden](https://github.com/eclipse/mosquitto/blob/master/mosquitto.conf) en te kopiëren naar de folder ``` ~/project/mosquitto/config ```.

```
docker run -it -p 1883:1883 -p 9001:9001 -v /Users/michael/Documents/KDG/IOT2/DiscoveringIOT/plantmonitor/mosquitto/config:/mosquitto/config --name mqtt eclipse-mosquitto
```

Een simpele helloworld kan getest worden door twee terminals te openen waar een subscriber zal luisteren naar een topic. Mockito heeft zelf een client en publisher CMD tool aan boord. Nu gezien we de setup in een docker container draaien en we deze CMD's niet vanop de host kunnen raadplegen dienen we twee interactieve terminals te open naar de docker instance genaamd ```mqtt```.
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
listener           9001
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

De Arduino zal luisteren naar de topics ```plantmonitor/actuators/emotion``` ```plantmonitor/actuators/waterpump``` ```plantmonitor/actuators/light``` en zal zelf berichten publiceren op de topic ```plantmonitor/sensors```


### Angular client code om te connecteren met de MQTT server

Voor de webapplicatie werd het Angular framework gekozen. Om via Angular naar de MQTT server te connecteren maken we gebruik van websockets en meer specifiek van de library [ngx-mqtt](https://www.npmjs.com/package/ngx-mqtt).

De belangrijkste logica gebeurt in de file ```app.component.ts```, die luistert naar de topic ```plantmonitor/sensors```. Via deze topic krijgt de webapplicatie volgende berichten binnen:
```
...
{"temp":22.0,"moisture":194,"light":4}
{"temp":22.0,"moisture":196,"light":7}
{"temp":22.0,"moisture":196,"light":7}
{"temp":22.0,"moisture":196,"light":10}
{"temp":22.0,"moisture":196,"light":6}
{"temp":22.0,"moisture":195,"light":5}
...
```

De waarden van deze drie sensoren worden in hun eigen buffer gestoken, die de laatste 20 waarden bijhoudt en gebruikt om berekening op uit te voeren en de UI dynamisch te laten veranderen. Meer info over de functionaliteit zal uitgelegd worden in de video demo.

De webapplicatie gaat ook zelf berichten publiceren op de topics:
-  ```plantmonitor/actuators/emotion``` om de LED Matrix emoties te laten veranderen op basis van de gemeten waarde. Te weinig water = unhappy face.
-  ```plantmonitor/actuators/waterpump``` kan je al gebruiker vanop afstand de plant watergeven. Dit stuurt de servo motor aan, bij gebrek aan het beschikken over een waterpompje.
-  ```plantmonitor/actuators/light``` kan je het lichtje aan en uit mee zetten, wanneer de plant te weinig licht zou hebben.


## Sensoren

### Temperatuur sensor

Voor de temperatuur sensor maken we gebruik van temperatuur sensor [TMP36 GZ](https://www.analog.com/media/en/technical-documentation/data-sheets/TMP35_36_37.pdf). Deze sensor heeft geen temperatuur gevoelige weerstand aan boord, maar meet de temperatuur wijzigingen door gebruik te maken van diodes. Als bij een diode de temperatuur verandert dan wijzigt ook de spanning volgens een gekende verhouding.

Het uitlezen van deze sensor (spanning meten we) geeft een waarde tussen 0 en 1023. Deze waarde moeten we dus gaan omzetten naar graden. In de Arduino opstelling wordt 3300 mV op de sensor gezet. In de datasheet lezen we dat de min temp -40°C to max 125 °C (165 range) is en dat wijzigingen van 10 mV overeenkomen met 1°C wijzigingen. Als -40°C overeenkomt met 0°C graden moeten we 400mv van de output voltage aftrekken.

```
 voltage = gemetenWaarde * (3300 / 1024) => voltage in mv
 temp = (voltage (mV)- 400 (mV)) / 10 (mV/°C) => temp in °C
```

Zoals reeds gemeld is het moeilijk te lezen of het nu over de TMP36 of TMP38. Na vergelijking met andere temperatuur sensoren benadert volgende vergelijking het best de temperatuur.

```
 voltage = gemetenWaarde * (3300 / 1024) => voltage in mv
 temp = (voltage (mV)- 500 (mV)) / 10 (mV/°C) => temp in °C
```

### Light sensors
Voor de lichtsensor maken we gebruik van een phototransistor en een weerstand van 1000 ohm. De keuze van de weerstand is bepaald door trial en error. Deze weerstand zorgt ervoor dat donker quasi 0 geeft en +- 1000 als je er een felle lamp naast houdt of vol zonlicht.

### DIY moisture sensor
We maken eigenlijk een potentiemeter na maar dan met de aarde. Net zoals bij een potentiemeter is de weerstand variabel tussen weerstand 1 en weerstand 2 (totale weerstand is dezelfde). En door te draaien aan een potentiemeter verander je de weerstands waarde van de weerstanden. Nu het draaien aan de potentiemeter gaat nu gesimuleerd worden door de vochtigheid van de aarde. Hoe vochtiger de aarde hoe lager de gemeten spanning zal zijn.

De treshold voor een plant die te droog is zetten we op 800 en een plant die verzuipt in het water moet een waarde hebben lager dan 100.

## Actuatoren

### Simpele LED
Een simpele LED die een UV lamp simuleert voor planten te laten groeien.

### Een LED 8X8 MATRIX
Hierbij berekenen we vanuit de webapplicatie de gemoedstoestand van de plant. Blij, niet blij, alsook error wanneer er een probleem is met de WiFi.

### Een servo motor
Bij gebrek aan een waterpomp geeft de wijzer van de servo motor aan of er water gegeven wordt of niet.
