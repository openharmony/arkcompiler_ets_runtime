/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { CallSign } from './call_sign';
import { Vector3D } from './vector_3d';
import { Aircraft } from './collision_detector';

const Z_NUM: number = 10;
const ADD_TIMES: number = 2;
const I_NUM: number = 3;

export class Simulator {
  aircraft: Array<CallSign> = [];

  constructor(numAircraft: number) {
    for (let i = 0; i < numAircraft; ++i) {
      this.aircraft.push(new CallSign('foo' + i));
    }
  }

  simulate(time: number): Array<Aircraft> {
    let frame: Array<Aircraft> = [];
    for (let i = 0; i < this.aircraft.length; i += ADD_TIMES) {
      frame.push(new Aircraft(this.aircraft[i], new Vector3D(time, Math.cos(time) * ADD_TIMES + i * I_NUM, Z_NUM)));
      frame.push(new Aircraft(this.aircraft[i + 1], new Vector3D(time, Math.sin(time) * ADD_TIMES + i * I_NUM, Z_NUM)));
    }
    return frame;
  }
}
