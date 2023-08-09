/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

class Body {
  constructor(x, vx, mass) {
    this.x = x;
    this.vx = vx;
    this.mass = mass;
  }
}

function advance(bodies, dt) {
  const sqrt = Math.sqrt;
  const size = bodies.length;

  for (let i = 0; i < size; i++) {
    const bodyi = bodies[i];
    let vxi = bodyi.vx;
    const xi = bodyi.x;
    const massi = bodyi.mass;

    for (let j = i + 1; j < size; j++) {
      const bodyj = bodies[j];
      const dx = xi - bodyj.x;

      const d2 = dx * dx;
      const mag = dt / (d2 * sqrt(d2));
      const massiMag = massi * mag;

      const massj = bodyj.mass;
      const massjMag = massj * mag;
      vxi -= dx * massjMag;

      bodyj.vx += dx * massiMag;
    }
    bodyi.vx = vxi;

    bodyi.x += dt * vxi;
  }
}

const PI = Math.PI;
const SOLAR_MASS = 4 * PI * PI;

function sun() {
  return new Body(0.0, 0.0, SOLAR_MASS);
}

function sun1() {
  return new Body(1.2, 3.1, PI);
}

const bodies = [sun(), sun1()];
const n = 100000;
for (let i = 0; i < n; i++) {
  advance(bodies, 0.01);
}
