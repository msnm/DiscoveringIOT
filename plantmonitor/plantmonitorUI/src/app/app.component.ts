import {Component, OnDestroy} from '@angular/core';
import {Subscription} from 'rxjs';
import {IMqttMessage, MqttService} from 'ngx-mqtt';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.sass']
})
export class AppComponent implements OnDestroy {
  title = 'Michael\'s IOT hub';

  private subscription: Subscription;
  public message: string;

  tempBuffer: number[] = [];
  moistureBuffer: number[] = [];
  lightBuffer: number[] = [];
  movement = false;

  constructor(private mqttService: MqttService) {
    this.message = 'Nothing is happening';
    this.subscription = this.mqttService.observe('plantmonitor/sensors').subscribe((message: IMqttMessage) => {
      this.message = message.payload.toString();
      this.addToResults(JSON.parse(this.message));
    });
  }

  private addToResults(response: JSON) {
    this.tempBuffer.unshift(response.temp);
    this.moistureBuffer.unshift(response.moisture);
    this.lightBuffer.unshift(response.light);
    console.log(this.lightBuffer);
    if (this.tempBuffer.length > 20) {
      this.tempBuffer.pop();
      this.moistureBuffer.pop();
      this.lightBuffer.pop();
    }

    if (Math.abs(this.lightBuffer[0] - this.lightBuffer[1]) > 40) {
      this.movement = !this.movement;
    } else {
      this.movement = false;
    }
  }

  public unsafePublish(topic: string, message: string): void {
    this.mqttService.unsafePublish(topic, message, {qos: 1, retain: true});
  }

  public giveWater(event) {
    this.unsafePublish('plantmonitor/actuators/waterpump', '1');
  }

  public ngOnDestroy() {
    this.subscription.unsubscribe();
  }
}
