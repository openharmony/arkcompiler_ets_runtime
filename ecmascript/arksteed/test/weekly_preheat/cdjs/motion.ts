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

import { Constants } from './constants';
import { CallSign } from './call_sign';
import { Vector3D } from './vector_3d';

const TIMES_NUM: number = 0.5;
const DOT_TWO: number = 2;
const DOT_FOUR: number = 4;

export class Motion {
  callSign: CallSign;
  posOne: Vector3D;
  posTwo: Vector3D;

  constructor(callSign: CallSign, posOne: Vector3D, posTwo: Vector3D) {
    this.callSign = callSign;
    this.posOne = posOne;
    this.posTwo = posTwo;
  }

  toString(): string {
    return 'Motion(' + this.callSign + ' from ' + this.posOne + ' to ' + this.posTwo + ')';
  }

  delta(): Vector3D {
    return this.posTwo.minus(this.posOne);
  }

  findIntersection(other: Motion): Vector3D | null {
    let init1 = this.posOne;
    let init2 = other.posOne;
    let vec1 = this.delta();
    let vec2 = other.delta();
    let radius = Constants.proximityRadius;
    let a = vec2.minus(vec1).squaredMagnitude();
    if (a !== 0) {
      let b = DOT_TWO * init1.minus(init2).dot(vec1.minus(vec2));
      let c = -radius * radius + init2.minus(init1).squaredMagnitude();
      let discr = b * b - DOT_FOUR * a * c;
      if (discr < 0) {
        return null;
      }
      let v1 = (-b - Math.sqrt(discr)) / (DOT_TWO * a);
      let v2 = (-b + Math.sqrt(discr)) / (DOT_TWO * a);

      if (v1 <= v2 && ((v1 <= 1 && 1 <= v2) || (v1 <= 0 && 0 <= v2) || (0 <= v1 && v2 <= 1))) {
        // Pick a good "time" at which to report the collision.
        let v: number = 0;
        if (v1 <= 0) {
          // The collision started before this frame. Report it at the start of the frame.
          v = 0;
        } else {
          // The collision started during this frame. Report it at that moment.
          v = v1;
        }

        let result1 = init1.plus(vec1.times(v));
        let result2 = init2.plus(vec2.times(v));

        let result = result1.plus(result2).times(TIMES_NUM);
        if (
          result.x >= Constants.minX &&
          result.x <= Constants.maxX &&
          result.y >= Constants.minY &&
          result.y <= Constants.maxY &&
          result.z >= Constants.minZ &&
          result.z <= Constants.maxZ
        ) {
          return result;
        }
      }
      return null;
    }
    let dist = init2.minus(init1).magnitude();
    if (dist <= radius) {
      return init1.plus(init2).times(TIMES_NUM);
    }
    return null;
  }
}
