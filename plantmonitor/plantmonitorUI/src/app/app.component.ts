import {Component, OnDestroy} from '@angular/core';
import {Subscription} from 'rxjs';
import {IMqttMessage, MqttService} from 'ngx-mqtt';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.sass']
})
export class AppComponent implements OnDestroy {
  private subscription: Subscription;
  needWater = false;
  private toMuchwater = false;
  private sleeping = false;
  toHot = false;
  private toCold = false;
  private toLight = false;
  private toDark = false;
  private movement = false;
  showRealValues = false;

  title = 'Michael\'s IOT hub';
  tempBuffer: number[] = [];
  moistureBuffer: number[] = [];
  lightBuffer: number[] = [];
  tempAvg: number;
  moistureAvg: number;
  lightAvg: number;
  moistureAvgPercentage: number;
  lightAvgPercentage: number;
  mood: string;
  lightOn = false;
  waterLedger: number[] = [];
  waving = false;
  private countWavingSeconds = 0;
  private countWaving = 0;

  constructor(private mqttService: MqttService) {
    this.mood = Mood.NO_CONNECTION;
    this.subscription = this.mqttService.observe('plantmonitor/sensors').subscribe((message: IMqttMessage) => {
      this.addToResults(JSON.parse(message.payload.toString()));
      this.interpretResults();
      this.feedback();
    });
  }

  private feedback() {
    if (!this.needWater || !this.toMuchwater || !this.toCold || !this.toLight || !this.toDark) {
      this.setEmotion('00'); // unhappy
    } else {
      this.setEmotion('11'); // happy
    }
  }
  private interpretResults() {
    let mood = Mood.OK.toString();
    const waterMood = this.calculateWaterMood();
    if (waterMood !== Mood.OK.toString()) {
      mood = waterMood;
    }
    const tempMood = this.calculateTempMood();
    if (tempMood !== Mood.OK.toString()) {
      mood = (mood !== Mood.OK.toString()) ? mood + ' ' + tempMood : tempMood;
    }
    const lightMood = this.calculateLightMood();

    if (lightMood !== Mood.OK.toString()) {
      mood = mood !== Mood.OK.toString() ? mood + ' ' + lightMood : lightMood;
    }

    const greetingMood = this.calculateGreetingMood();
    if (greetingMood !== Mood.OK.toString()) {
      mood =  greetingMood;
    }
    this.mood = mood;
  }

  private calculateLightMood() {
    if (this.lightAvg < 20 ) {
      this.toLight = true;
      this.toDark = false;
      return Mood.TO_DARK;
    } else if (this.lightAvg > 800)  {
      this.toLight = true;
      this.toDark = false;
      return Mood.TO_LIGHT;
    } else {
      this.toLight = true;
      this.toDark = false;
      return Mood.OK;
    }
  }

  private calculateGreetingMood() {
    if (this.waving) {
      this.countWavingSeconds = this.countWavingSeconds + 1;
      console.log(this.countWavingSeconds);
    }
    if (this.waving === true && this.countWavingSeconds < 10) {
      return Mood.GREETING;
    } else if (this.waving === true && this.countWavingSeconds > 10) {
      this.waving = false;
      this.countWavingSeconds = 0;
      return Mood.OK;
    } else if (Math.abs(this.lightBuffer[0] - this.lightBuffer[1]) > 50) {
      this.countWaving = this.countWaving + 1;
      console.log(this.countWaving);
      if (this.countWaving > 3) {
        this.waving = true;
        this.countWaving = 0;
        return Mood.GREETING;
      }
    }
    return Mood.OK;
  }
  private calculateWaterMood() {
    if (this.moistureAvg < 100 ) {
      this.toMuchwater = true;
      this.needWater = false;
      return Mood.DRUNK;
    } else if (this.moistureAvg > 800)  {
      this.needWater = true;
      this.toMuchwater = false;
      return Mood.THURSTY;
    } else {
      this.needWater = false;
      this.toMuchwater = false;
      return Mood.OK;
    }
  }

  private calculateTempMood() {
    if (this.tempAvg < 15 ) {
      this.toCold = true;
      this.toHot = false;
      return Mood.TO_COLD;
    } else if (this.tempAvg > 35)  {
      this.toHot = true;
      this.toCold = false;
      return Mood.TO_HOT;
    } else {
      this.toHot = false;
      this.toCold = false;
      return Mood.OK;
    }
  }

  private addToResults(response) {
    this.tempBuffer.unshift(response.temp);
    this.moistureBuffer.unshift(response.moisture);
    this.lightBuffer.unshift(response.light);
    if (this.tempBuffer.length > 20) {
      this.tempBuffer.pop();
      this.moistureBuffer.pop();
      this.lightBuffer.pop();
    }
    this.moistureAvg = this.moistureBuffer.reduce((x, y) => x + y) / this.moistureBuffer.length;
    this.lightAvg = this.lightBuffer.reduce((x, y) => x + y) / this.lightBuffer.length;
    this.tempAvg = Math.round(this.tempBuffer.reduce((x, y) => x + y) / this.tempBuffer.length);
    this.moistureAvgPercentage = Math.round((1 - this.moistureAvg / 1024) * 100);
    this.lightAvgPercentage = Math.round((this.lightAvg / 1024) * 100);

  }

  public unsafePublish(topic: string, message: string): void {
    this.mqttService.unsafePublish(topic, message, {qos: 1, retain: true});
  }

  public giveWater(event) {
    this.unsafePublish('plantmonitor/actuators/waterpump', '1');
    this.waterLedger.unshift(Date.now());
  }

  public turnOnTheLight(event) {
    this.unsafePublish('plantmonitor/actuators/light', '1');
    this.lightOn = true;

  }

  public turnOffTheLight(event) {
    this.unsafePublish('plantmonitor/actuators/light', '0');
    this.lightOn = false;
  }


  public switchValueRepresentation(event) {
    this.showRealValues = !this.showRealValues;
  }

  public setEmotion(emotion) {
    this.unsafePublish('plantmonitor/actuators/emotion', emotion);
  }

  public ngOnDestroy() {
    this.subscription.unsubscribe();
  }
}

enum Mood {
  OK = 'I \'m absolutely fine, thanks for asking!',
  TO_HOT = 'Pfff, is this the summer of 69?',
  TO_COLD = 'Wow my feets are freezing off, please put me inside!',
  TO_LIGHT = 'Do you have some sunscreen or sunshade?',
  TO_DARK = 'I know you are my sunshine in life, but please put me aside a free window!',
  THURSTY = 'May I also have a glass of that bottle?',
  DRUNK = 'Maybe I drank a bit too much, can you check if my feet are not wet?',
  GREETING = 'Hello Friend, thanks for waving. I hope you had a great day. ',
  NO_CONNECTION = 'No connection'
}
