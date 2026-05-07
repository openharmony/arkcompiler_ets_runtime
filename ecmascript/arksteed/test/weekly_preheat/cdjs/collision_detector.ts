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

import { RedBlackTree } from './red_black_tree';
import { Motion } from './motion';
import { Collision } from './collision';
import { reduceCollisionSet } from './reduce_collision_set';
import { CallSign } from './call_sign';
import { Vector3D } from './vector_3d';

export class Aircraft {
  callSign: CallSign;
  position: Vector3D;

  constructor(callSign: CallSign, position: Vector3D) {
    this.callSign = callSign;
    this.position = position;
  }
}

export class CollisionDetector {
  state: RedBlackTree = new RedBlackTree();

  handleNewFrame(frame: Array<Aircraft>): Array<Collision> {
    let motions: Array<Motion> = [];
    let seen: RedBlackTree = new RedBlackTree();

    for (const aircraft of frame) {
      let oldPosition = this.state.put(aircraft.callSign, aircraft.position) as Vector3D;
      let newPosition = aircraft.position;
      seen.put(aircraft.callSign, true);

      if (oldPosition === null) {
        // Treat newly introduced aircraft as if they were stationary.
        oldPosition = newPosition;
      }

      motions.push(new Motion(aircraft.callSign, oldPosition, newPosition));
    }

    // Remove aircraft that are no longer present.
    let toRemove: Array<CallSign> = [];
    this.state.forEach((callSign, position) => {
      if (seen.get(callSign) === null) {
        toRemove.push(callSign as CallSign);
      }
    });
    for (const callSign of toRemove) {
      this.state.remove(callSign);
    }

    const allReduced = reduceCollisionSet(motions);
    let collisions: Array<Collision> = [];
    for (const reduced of allReduced) {
      for (let i = 0; i < reduced.length; ++i) {
        let motion1 = reduced[i];
        for (let j = i + 1; j < reduced.length; ++j) {
          let motion2 = reduced[j];
          let collision = motion1.findIntersection(motion2);
          if (collision !== null) {
            collisions.push(new Collision([motion1.callSign, motion2.callSign], collision));
          }
        }
      }
    }

    return collisions;
  }
}
