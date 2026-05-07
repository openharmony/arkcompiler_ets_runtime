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

const COMMON_NUMBER_FOUR: number = 4;
const CURRENT_LOOP_STEP: number = 2;
const CURRENT_LOOP_COUNT: number = 24;
const CURRENT_LOOP_OPERATE_NUMBER: number = 100;
const FUNCTION_ADVANCE_ARGUMENT_DT: number = 0.01;
const FUNCTION_ENERGY_OPERATE_COEFFICIENT: number = 0.5;
const MAX_LOOP_COUNT: number = 8;
const MS_CONVERSION_RATIO: number = 1000;

const INSTANCE_BODY_ONE_X: number = 4.8414314424647209;
const INSTANCE_BODY_ONE_Y: number = -1.16032004402742839;
const INSTANCE_BODY_ONE_Z: number = -1.03622044471123109e-1;
const INSTANCE_BODY_ONE_VX: number = 1.66007664274403694e-3;
const INSTANCE_BODY_ONE_VY: number = 7.69901118419740425e-3;
const INSTANCE_BODY_ONE_VZ: number = -6.90460016972063023e-5;
const INSTANCE_BODY_ONE_MASS: number = 9.54791938424326609e-4;

const INSTANCE_BODY_TWO_X: number = 8.34336671824457987;
const INSTANCE_BODY_TWO_Y: number = 4.12479856412430479;
const INSTANCE_BODY_TWO_Z: number = -4.03523417114321381e-1;
const INSTANCE_BODY_TWO_VX: number = -2.76742510726862411e-3;
const INSTANCE_BODY_TWO_VY: number = 4.99852801234917238e-3;
const INSTANCE_BODY_TWO_VZ: number = 2.30417297573763929e-5;
const INSTANCE_BODY_TWO_MASS: number = 2.85885980666130812e-4;

const INSTANCE_BODY_THREE_X: number = 1.2894369562139131e1;
const INSTANCE_BODY_THREE_Y: number = -1.51111514016986312e1;
const INSTANCE_BODY_THREE_Z: number = -2.23307578892655734e-1;
const INSTANCE_BODY_THREE_VX: number = 2.96460137564761618e-3;
const INSTANCE_BODY_THREE_VY: number = 2.3784717395948095e-3;
const INSTANCE_BODY_THREE_VZ: number = -2.96589568540237556e-5;
const INSTANCE_BODY_THREE_MASS: number = 4.36624404335156298e-5;

const INSTANCE_BODY_FOUR_X: number = 1.53796971148509165e1;
const INSTANCE_BODY_FOUR_Y: number = -2.59193146099879641e1;
const INSTANCE_BODY_FOUR_Z: number = 1.79258772950371181e-1;
const INSTANCE_BODY_FOUR_VX: number = 2.68067772490389322e-3;
const INSTANCE_BODY_FOUR_VY: number = 1.62824170038242295e-3;
const INSTANCE_BODY_FOUR_VZ: number = -9.5159225451971587e-5;
const INSTANCE_BODY_FOUR_MASS: number = 5.15138902046611451e-5;

let PI: number = 3.141592653589793;
let SOLAR_MASS: number = COMMON_NUMBER_FOUR * PI * PI;
let DAYS_PER_YEAR: number = 365.24;

class Body {
  x: number;
  y: number;
  z: number;
  vx: number;
  vy: number;
  vz: number;
  mass: number;

  constructor(x1: number, y1: number, z1: number, vx: number, vy: number, vz: number, mass: number) {
    this.x = x1;
    this.vx = vx;
    this.y = y1;
    this.vy = vy;
    this.z = z1;
    this.vz = vz;
    this.mass = mass;
  }

  offsetMomentum(px: number, py: number, pz: number): Body {
    this.vx = -px / SOLAR_MASS;
    this.vy = -py / SOLAR_MASS;
    this.vz = -pz / SOLAR_MASS;
    return this;
  }
}

function jupiter(): Body {
  return new Body(
    INSTANCE_BODY_ONE_X,
    INSTANCE_BODY_ONE_Y,
    INSTANCE_BODY_ONE_Z,
    INSTANCE_BODY_ONE_VX * DAYS_PER_YEAR,
    INSTANCE_BODY_ONE_VY * DAYS_PER_YEAR,
    INSTANCE_BODY_ONE_VZ * DAYS_PER_YEAR,
    INSTANCE_BODY_ONE_MASS * SOLAR_MASS
  );
}

function saturn(): Body {
  return new Body(
    INSTANCE_BODY_TWO_X,
    INSTANCE_BODY_TWO_Y,
    INSTANCE_BODY_TWO_Z,
    INSTANCE_BODY_TWO_VX * DAYS_PER_YEAR,
    INSTANCE_BODY_TWO_VY * DAYS_PER_YEAR,
    INSTANCE_BODY_TWO_VZ * DAYS_PER_YEAR,
    INSTANCE_BODY_TWO_MASS * SOLAR_MASS
  );
}

function uranus(): Body {
  return new Body(
    INSTANCE_BODY_THREE_X,
    INSTANCE_BODY_THREE_Y,
    INSTANCE_BODY_THREE_Z,
    INSTANCE_BODY_THREE_VX * DAYS_PER_YEAR,
    INSTANCE_BODY_THREE_VY * DAYS_PER_YEAR,
    INSTANCE_BODY_THREE_VZ * DAYS_PER_YEAR,
    INSTANCE_BODY_THREE_MASS * SOLAR_MASS
  );
}

function neptune(): Body {
  return new Body(
    INSTANCE_BODY_FOUR_X,
    INSTANCE_BODY_FOUR_Y,
    INSTANCE_BODY_FOUR_Z,
    INSTANCE_BODY_FOUR_VX * DAYS_PER_YEAR,
    INSTANCE_BODY_FOUR_VY * DAYS_PER_YEAR,
    INSTANCE_BODY_FOUR_VZ * DAYS_PER_YEAR,
    INSTANCE_BODY_FOUR_MASS * SOLAR_MASS
  );
}

function sun(): Body {
  return new Body(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, SOLAR_MASS);
}

class NBodySystem {
  bodies: Body[];

  constructor(bodies: Body[]) {
    this.bodies = bodies;
    let px = 0.0;
    let py = 0.0;
    let pz = 0.0;
    let size = this.bodies.length;
    for (let i = 0; i < size; i++) {
      let b = this.bodies[i];
      let m = b.mass;
      px += b.vx * m;
      py += b.vy * m;
      pz += b.vz * m;
    }

    this.bodies[0].offsetMomentum(px, py, pz);
  }

  advance(dt: number): void {
    let dx: number;
    let dy: number;
    let dz: number;
    let distance: number;
    let mag: number;
    let size = this.bodies.length;

    for (let i = 0; i < size; i++) {
      let bodyI = this.bodies[i];
      for (let j = i + 1; j < size; j++) {
        let bodyJ = this.bodies[j];
        dx = bodyI.x - bodyJ.x;
        dy = bodyI.y - bodyJ.y;
        dz = bodyI.z - bodyJ.z;

        distance = Math.sqrt(dx * dx + dy * dy + dz * dz);
        mag = dt / (distance * distance * distance);

        bodyI.vx -= dx * bodyJ.mass * mag;
        bodyI.vy -= dy * bodyJ.mass * mag;
        bodyI.vz -= dz * bodyJ.mass * mag;

        bodyJ.vx += dx * bodyI.mass * mag;
        bodyJ.vy += dy * bodyI.mass * mag;
        bodyJ.vz += dz * bodyI.mass * mag;
      }
    }

    for (let i = 0; i < size; i++) {
      let body = this.bodies[i];
      body.x += dt * body.vx;
      body.y += dt * body.vy;
      body.z += dt * body.vz;
    }
  }

  energy(): number {
    let dx: number;
    let dy: number;
    let dz: number;
    let distance: number;
    let e = 0.0;
    let size = this.bodies.length;

    for (let i = 0; i < size; i++) {
      let bodyI = this.bodies[i];

      e += FUNCTION_ENERGY_OPERATE_COEFFICIENT * bodyI.mass * (bodyI.vx * bodyI.vx + bodyI.vy * bodyI.vy + bodyI.vz * bodyI.vz);

      for (let j = i + 1; j < size; j++) {
        let bodyJ = this.bodies[j];
        dx = bodyI.x - bodyJ.x;
        dy = bodyI.y - bodyJ.y;
        dz = bodyI.z - bodyJ.z;
        distance = Math.sqrt(dx * dx + dy * dy + dz * dz);
        e -= (bodyI.mass * bodyJ.mass) / distance;
      }
    }
    return e;
  }
}

declare interface ArkTools {
  timeInUs(args: number): number;
}

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  /*
   *@Benchmark
   */
  run(): void {
    let ret = 0;
    let n = 3;
    while (n <= CURRENT_LOOP_COUNT) {
      let planets = [sun(), jupiter(), saturn(), uranus(), neptune()];
      let bodies = new NBodySystem(planets);
      let max = n * CURRENT_LOOP_OPERATE_NUMBER;
      ret += bodies.energy();

      for (let i = 0; i < max; i++) {
        bodies.advance(FUNCTION_ADVANCE_ARGUMENT_DT);
      }
      ret += bodies.energy();
      n *= CURRENT_LOOP_STEP;
    }

    let expected = -1.3524862408537381;
    if (ret !== expected) {
      print('ERROR: bad result: expected' + expected + 'but got' + ret);
    }
  }

  runIterationTime(): void {
    let start = ArkTools.timeInUs();
    for (let i = 0; i < MAX_LOOP_COUNT; i++) {
      this.run();
    }
    let end = ArkTools.timeInUs();
    print('n-body: ms = ' + (end - start) / MS_CONVERSION_RATIO);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().runIterationTime();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().runIterationTime();
