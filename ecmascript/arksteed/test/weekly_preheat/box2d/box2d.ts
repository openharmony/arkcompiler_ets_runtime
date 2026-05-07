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
import { B2PolygonShape } from './Collision/Shapes/b2Shape';
import { addEqual, B2Vec2 } from './Common/b2Math';
import { B2BodyDef, B2BodyType } from './Dynamics/b2Body';
import { B2FixtureDef } from './Dynamics/b2Fixture';
import { B2World } from './Dynamics/b2World';

/*
 * @State
 * @Tags Jetstream2
 */
export class Benchmark {
  /*
   * @Benchmark
   */
  benchmarkRun(): void {
    let loop120: number = 20;
    let multiple: number = 1000.0;
    let start = ArkTools.timeInUs() / multiple;
    for (let i = 0; i < loop120; i++) {
      this.runIteration();
    }
    let end = ArkTools.timeInUs() / multiple;
    print('box2d: ms = ' + (end - start));
  }

  runIteration(): void {
    this.runBox2D();
  }

  runBox2D(): void {
    let times: number = 60.0;
    let second: number = 1.0;
    let loop20: number = 20;
    let velocityIterations: number = 10;
    let positionIterations: number = 3;
    let world = this.makeNewWorld();
    for (let i = 0; i < loop20; i++) {
      world.step(second / times, velocityIterations, positionIterations);
      // log('position_y = :' + world.mBodyList?.position.y);
    }
  }

  /*
   * @Setup
   */
  makeNewWorld(): B2World {
    let loop5: number = 5;
    let loop10: number = 10;
    let density: number = 5.0;
    let xX: number = -7.0;
    let xY: number = 0.75;
    let gravityY: number = -10.0;
    let zero: number = 0.0;
    let edgeV1X: number = -40.0;
    let edgeV2X: number = 40.0;
    let deltaXX: number = 0.5625;
    let deltaXY: number = 1.0;
    let deltaYX: number = 1.125;
    let gravity = new B2Vec2(zero, gravityY);
    let world = new B2World(gravity);
    let shape = new B2PolygonShape();
    shape.setAsEdge(new B2Vec2(edgeV1X, zero), new B2Vec2(edgeV2X, zero));
    shape.radius = 0;
    let fd = new B2FixtureDef();
    fd.density = zero;
    fd.shape = shape;
    let bd = new B2BodyDef();
    bd.type = B2BodyType.STATICBODY;
    let ground = world.createBody(bd);
    ground.createFixture(fd);
    let a: number = 0.5;
    let shape2 = new B2PolygonShape();
    shape2.setAsBox(a, a);
    shape2.radius = 0;
    let x = new B2Vec2(xX, xY);
    let y = new B2Vec2();
    let deltaX = new B2Vec2(deltaXX, deltaXY);
    let deltaY = new B2Vec2(deltaYX, zero);
    for (let i = 0; i < loop10; i++) {
      y.set(x.x, x.y);
      for (let j = 0; j < loop5; j++) {
        let fd = new B2FixtureDef();
        fd.density = density;
        fd.shape = shape2;
        let bd = new B2BodyDef();
        bd.type = B2BodyType.DYNAMICBODY;
        bd.position.set(y.x, y.y);
        let body = world.createBody(bd);
        body.createFixture(fd);
        addEqual(y, deltaY);
      }
      addEqual(x, deltaX);
    }
    return world;
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().benchmarkRun();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().benchmarkRun();

let debug: boolean = false;
function log(msg: string): void {
  if (debug) {
    print(msg);
  }
}

declare interface ArkTools {
  timeInUs(args: number): number;
}
