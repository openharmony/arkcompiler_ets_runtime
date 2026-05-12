/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

/* The Computer Language Benchmarks Game
   https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
   contributed by Isaac Gouy
   modified by Andrey Filatkin */

//    import { BenchmarkRunner } from "../../../utils/benchmarkTsSuite";
// declare function print(arg:string):string;
//
declare interface ArkTools {
  timeInUs(arg:any):number
}
const PI = Math.PI;
const SOLAR_MASS = 4 * PI * PI;
const DAYS_PER_YEAR = 365.24;

class Body {
  x: number;
  y: number;
  z: number;
  vx: number;
  vy: number;
  vz: number;
  mass: number;
  constructor(x: number, y: number, z: number, vx: number, vy: number, vz: number, mass: number) {
    this.x = x;
    this.y = y;
    this.z = z;
    this.vx = vx;
    this.vy = vy;
    this.vz = vz;
    this.mass = mass;
  }
}

function Jupiter() {
  return new Body(
    4.84143144246472090e+00,
    -1.16032004402742839e+00,
    -1.03622044471123109e-01,
    1.66007664274403694e-03 * DAYS_PER_YEAR,
    7.69901118419740425e-03 * DAYS_PER_YEAR,
    -6.90460016972063023e-05 * DAYS_PER_YEAR,
    9.54791938424326609e-04 * SOLAR_MASS
  );
}

function Saturn() {
  return new Body(
    8.34336671824457987e+00,
    4.12479856412430479e+00,
    -4.03523417114321381e-01,
    -2.76742510726862411e-03 * DAYS_PER_YEAR,
    4.99852801234917238e-03 * DAYS_PER_YEAR,
    2.30417297573763929e-05 * DAYS_PER_YEAR,
    2.85885980666130812e-04 * SOLAR_MASS
  );
}

function Uranus() {
  return new Body(
    1.28943695621391310e+01,
    -1.51111514016986312e+01,
    -2.23307578892655734e-01,
    2.96460137564761618e-03 * DAYS_PER_YEAR,
    2.37847173959480950e-03 * DAYS_PER_YEAR,
    -2.96589568540237556e-05 * DAYS_PER_YEAR,
    4.36624404335156298e-05 * SOLAR_MASS
  );
}

function Neptune() {
  return new Body(
    1.53796971148509165e+01,
    -2.59193146099879641e+01,
    1.79258772950371181e-01,
    2.68067772490389322e-03 * DAYS_PER_YEAR,
    1.62824170038242295e-03 * DAYS_PER_YEAR,
    -9.51592254519715870e-05 * DAYS_PER_YEAR,
    5.15138902046611451e-05 * SOLAR_MASS
  );
}

function Sun() {
  return new Body(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, SOLAR_MASS);
}

const bodies: Body[] = [Sun(), Jupiter(), Saturn(), Uranus(), Neptune()];

function offsetMomentum(bodies: Body[]) {
  let px = 0.0;
  let py = 0.0;
  let pz = 0.0;
  const size = bodies.length;
  for (let i = 0; i < size; i++) {
    const body: Body = bodies[i];
    const mass = body.mass;
    px += body.vx * mass;
    py += body.vy * mass;
    pz += body.vz * mass;
  }

  const body = bodies[0];
  body.vx = -px / SOLAR_MASS;
  body.vy = -py / SOLAR_MASS;
  body.vz = -pz / SOLAR_MASS;
}

function advance(bodies: Body[], dt: number) {
  const sqrt = Math.sqrt;
  const size = bodies.length;
  for (let i = 0; i < size; i++) {
    const bodyi: Body = bodies[i];
    let vxi = bodyi.vx;
    let vyi = bodyi.vy;
    let vzi = bodyi.vz;
    const xi = bodyi.x;
    const yi = bodyi.y;
    const zi = bodyi.z;
    const massi = bodyi.mass;

    for (let j = i + 1; j < size; j++) {
      const bodyj: Body = bodies[j];
      const dx = xi - bodyj.x;
      const dy = yi - bodyj.y;
      const dz = zi - bodyj.z;

      const d2 = dx * dx + dy * dy + dz * dz;
      const mag = dt / (d2 * sqrt(d2));
      //const mag = dt / (d2 * d2);
      const massiMag = massi * mag;

      const massj = bodyj.mass;
      const massjMag = massj * mag;
      vxi -= dx * massjMag;
      vyi -= dy * massjMag;
      vzi -= dz * massjMag;

      bodyj.vx += dx * massiMag;
      bodyj.vy += dy * massiMag;
      bodyj.vz += dz * massiMag;
    }
    bodyi.vx = vxi;
    bodyi.vy = vyi;
    bodyi.vz = vzi;

    bodyi.x += dt * vxi;
    bodyi.y += dt * vyi;
    bodyi.z += dt * vzi;
  }
}
function energy(bodies: Body[]) {
  let e = 0.0;
  const size = bodies.length;

  for (let i = 0; i < size; i++) {
    const bodyi = bodies[i];

    e += 0.5 * bodyi.mass * (bodyi.vx * bodyi.vx + bodyi.vy * bodyi.vy + bodyi.vz * bodyi.vz);

    for (let j = i + 1; j < size; j++) {
      const bodyj = bodies[j];
      const dx = bodyi.x - bodyj.x;
      const dy = bodyi.y - bodyj.y;
      const dz = bodyi.z - bodyj.z;

      const distance = Math.sqrt(dx * dx + dy * dy + dz * dz);
      e -= (bodyi.mass * bodyj.mass) / distance;
    }
  }
  return e;
}

export function RunNBody() {
  const n = 1000000;
  energy(bodies);
  let start = ArkTools.timeInUs();
  for (let i = 0; i < n; i++) {
    advance(bodies, 0.01);
  }
  let time = (ArkTools.timeInUs() - start) / 1000
  print("NBody - RunNBody: \t" + String(time) + "\tms" )
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunNBody()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunNBody()
//    let runner1 = new BenchmarkRunner("NBody - RunNBody", RunNBody);
//    runner1.run();

