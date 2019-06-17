import { observable, action } from 'mobx';

import PanelState from './panel-state';

export class LidarFrame {
  constructor(message) {
    const { data } = message;
    const {
      angleStart,
      angleEnd,
      angleDelta,
      timeDelta,
      scanTime,
      rangeMin,
      rangeMax,
      ranges,
      intensities,
      timestamp,
    } = data;
    this.angleStart = angleStart;
    this.angleEnd = angleEnd;
    this.angleDelta = angleDelta;
    this.timeDelta = timeDelta;
    this.scanTime = scanTime;
    this.rangeMin = rangeMin;
    this.rangeMax = rangeMax;
    this.ranges = ranges;
    this.intensities = intensities;
    this.timestamp = timestamp;
  }
}

export default class LidarPanelState extends PanelState {
  @observable frame = null;

  @action update(message) {
    this.frame = new LidarFrame(message);
  }

  type = () => {
    return 'LidarPanel';
  };
}