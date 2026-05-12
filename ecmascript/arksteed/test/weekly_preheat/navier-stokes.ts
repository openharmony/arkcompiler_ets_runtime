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

let solver: FluidField;
let nsFrameCounter: number = 0;
let factor: number = 0.5;
let densFactor: number = 10;
let point: number = 64;
let repeatCount: number = 15;
let checkIndex: number = 7000;
let checkIndexTo: number = 7100;
let indexTwo: number = 2;
let point2: number = 128;
let resultOK: number = 77;
let densInt0: number = 5;
let densInt1: number = 20;
let densInt2: number = 30;
let itersMax: number = 100;
let pointMax: number = 1000000;
let solveNumber: number = 4;
let multiple: number = 1000.0;
let runCount: number = 120;

function runNavierStokes(): void {
  solver?.update();
  nsFrameCounter += 1;
  if (nsFrameCounter === repeatCount) {
    checkResult(solver.getDens());
  }
}

function checkResult(dens: Float64Array): void {
  let result: number = 0;
  for (let i = checkIndex; i < checkIndexTo; i++) {
    result += ~~(dens[i] * densFactor);
    //log('single result = ' + dens[i] * densFactor);
  }
  //log('checkResult result = ' + result);
  if (result !== resultOK) {
    //log('nsFrameCounter=' + nsFrameCounter + 'checksum failed');
  } else {
    //log('nsFrameCounter=' + nsFrameCounter + 'checksum success');
  }
}
/*
 * @Setup
 */
function setupNavierStokes(): void {
  solver = new FluidField();
  solver?.setResolution(point2, point2);
  solver?.setIterations(densInt1);
  solver?.reset();
}
let framesTillAddingPoints = 0;
let framesBetweenAddingPoints = 5;

function prepareFrame(d: Float64Array, u: Float64Array, v: Float64Array): void {
  if (framesTillAddingPoints === 0) {
    solver?.addPoints(d, u, v);
    framesTillAddingPoints = framesBetweenAddingPoints;
    framesBetweenAddingPoints += 1;
  } else {
    framesTillAddingPoints -= 1;
  }
}

class FluidField {
  addFields(x: Float64Array, s: Float64Array, dt: number): void {
    for (let i = 0; i < this.size!; i++) {
      x[i] += dt * s[i];
    }
  }

  setBnd(b: number, x: Float64Array): void {
    if (b === 1) {
      for (let i = 1; i <= this.width; i++) {
        x[i] = x[i + this.rowSize];
        x[i + (this.height + 1) * this.rowSize] = x[i + this.height * this.rowSize];
      }

      for (let j = 1; j <= this.height; j++) {
        x[j * this.rowSize] = -x[1 + j * this.rowSize];
        x[this.width + 1 + j * this.rowSize] = -x[this.width + j * this.rowSize];
      }
    } else if (b === indexTwo) {
      for (let i = 1; i <= this.width; i++) {
        x[i] = -x[i + this.rowSize];
        x[i + (this.height + 1) * this.rowSize] = -x[i + this.height * this.rowSize];
      }

      for (let j = 1; j <= this.height; j++) {
        x[j * this.rowSize] = x[1 + j * this.rowSize];
        x[this.width + 1 + j * this.rowSize] = x[this.width + j * this.rowSize];
      }
    } else {
      for (let i = 1; i <= this.width; i++) {
        x[i] = x[i + this.rowSize];
        x[i + (this.height + 1) * this.rowSize] = x[i + this.height * this.rowSize];
      }

      for (let j = 1; j <= this.height; j++) {
        x[j * this.rowSize] = x[1 + j * this.rowSize];
        x[this.width + 1 + j * this.rowSize] = x[this.width + j * this.rowSize];
      }
    }
    let maxEdge: number = (this.height + 1) * this.rowSize;
    x[0] = factor * (x[1] + x[this.rowSize]);
    x[maxEdge] = factor * (x[1 + maxEdge] + x[this.height * this.rowSize]);
    x[this.width + 1] = factor * (x[this.width] + x[this.width + 1 + this.rowSize]);
    x[this.width + 1 + maxEdge] = factor * (x[this.width + maxEdge] + x[this.width + 1 + this.height * this.rowSize]);
  }

  linSolve(b: number, x: Float64Array, x0: Float64Array, a: number, c: number): void {
    if (a === 0 && c === 1) {
      for (let j = 1; j <= this.height; j++) {
        let currentRow: number = j * this.rowSize;
        currentRow += 1;
        for (let i = 0; i < this.width; i++) {
          x[currentRow] = x0[currentRow];
          currentRow += 1;
        }
      }
      this.setBnd(b, x);
    } else {
      let invC: number = 1 / c;
      for (let k = 0; k < this.iterations; k++) {
        this.linSolveElse(x, x0, a, invC);
        this.setBnd(b, x);
      }
    }
  }
  linSolveElse(x: Float64Array, x0: Float64Array, a: number, invC: number): void {
    for (let j = 1; j <= this.height; j++) {
      let lastRow = (j - 1) * this.rowSize;
      let currentRow = j * this.rowSize;
      let nextRow = (j + 1) * this.rowSize;
      let lastX: number = x[currentRow];
      let changeRow = currentRow;
      currentRow += 1;
      for (let i = 1; i <= this.width; i++) {
        let args1 = x0[currentRow];
        changeRow = currentRow;
        currentRow += 1;
        lastRow += 1;
        nextRow += 1;
        let args2 = a * (lastX + x[currentRow] + x[lastRow] + x[nextRow]);
        x[changeRow] = (args1 + args2) * invC;
        lastX = x[changeRow];
      }
    }
  }
  diffuse(b: number, x: Float64Array, x0: Float64Array): void {
    let a = 0;
    this.linSolve(b, x, x0, a, 1 + solveNumber * a);
  }

  linSolve2(x: Float64Array, x0: Float64Array, y: Float64Array, y0: Float64Array, a: number, c: number): void {
    if (a === 0 && c === 1) {
      for (let j = 1; j <= this.height; j++) {
        let currentRow = j * this.rowSize;
        currentRow += 1;
        for (let i = 0; i < this.width; i++) {
          x[currentRow] = x0[currentRow];
          y[currentRow] = y0[currentRow];
          currentRow += 1;
        }
      }
      this.setBnd(1, x);
      this.setBnd(indexTwo, y);
    } else {
      let invC: number = 1 / c;
      for (let k = 0; k < this.iterations; k++) {
        this.linSolve2Else(x, x0, y, y0, a, invC);
        this.setBnd(1, x);
        this.setBnd(indexTwo, y);
      }
    }
  }
  linSolve2Else(x: Float64Array, x0: Float64Array, y: Float64Array, y0: Float64Array, a: number, invC: number): void {
    for (let j = 1; j <= this.height; j++) {
      let lastRow = (j - 1) * this.rowSize;
      let currentRow = j * this.rowSize;
      let nextRow = (j + 1) * this.rowSize;
      let lastX = x[currentRow];
      let lastY = y[currentRow];
      currentRow += 1;
      for (let i = 1; i <= this.width; i++) {
        currentRow += 1;
        lastRow += 1;
        nextRow += 1;
        let test = x0[currentRow] + a * (lastX + x[currentRow] + x[lastRow] + x[nextRow]);
        x[currentRow] = test * invC;
        lastX = x[currentRow];
        let testy = y0[currentRow] + a * (lastY + y[currentRow] + y[lastRow] + y[nextRow]);
        y[currentRow] = testy * invC;
        lastY = y[currentRow];
      }
    }
  }
  diffuse2(x: Float64Array, x0: Float64Array, y: Float64Array, y0: Float64Array): void {
    let a = 0;
    this.linSolve2(x, x0, y, y0, a, 1 + solveNumber * a);
  }

  advect(b: number, d: Float64Array, d0: Float64Array, u: Float64Array, v: Float64Array, dt: number): void {
    let wdt0 = dt * this.width;
    let hdt0 = dt * this.height;
    let wp5 = this.width + factor;
    let hp5 = this.height + factor;
    for (let j = 1; j <= this.height; j++) {
      let pos = j * this.rowSize;
      for (let i = 1; i <= this.width; i++) {
        pos += 1;
        let x = i - wdt0 * u[pos];
        let y = j - hdt0 * v[pos];
        if (x < factor) {
          x = factor;
        } else if (x > wp5) {
          x = wp5;
        }
        let i0 = x | 0;
        let i1 = i0 + 1;
        if (y < factor) {
          y = factor;
        } else if (y > hp5) {
          y = hp5;
        }
        let j0 = y | 0;
        let j1 = j0 + 1;
        let s1 = x - i0;
        let s0 = 1 - s1;
        let t1 = y - j0;
        let t0 = 1 - t1;
        let row1 = j0 * this.rowSize;
        let row2 = j1 * this.rowSize;
        d[pos] = s0 * (t0 * d0[i0 + row1] + t1 * d0[i0 + row2]) + s1 * (t0 * d0[i1 + row1] + t1 * d0[i1 + row2]);
      }
    }
    this.setBnd(b, d);
  }

  project(u: Float64Array, v: Float64Array, p: Float64Array, div: Float64Array): void {
    let h = -factor / Math.sqrt(this.width * this.height);
    for (let j = 1; j <= this.height; j++) {
      let row = j * this.rowSize;
      let previousRow = (j - 1) * this.rowSize;
      let prevValue = row - 1;
      let currentRow = row;
      let nextValue = row + 1;
      let nextRow = (j + 1) * this.rowSize;
      for (let i = 1; i <= this.width; i++) {
        currentRow += 1;
        nextValue += 1;
        nextRow += 1;
        previousRow += 1;
        prevValue += 1;
        div[currentRow] = h * (u[nextValue] - u[prevValue] + v[nextRow] - v[previousRow]);
        p[currentRow] = 0;
      }
    }
    this.setBnd(0, div);
    this.setBnd(0, p);
    this.linSolve(0, p, div, 1, solveNumber);
    let wScale = factor * this.width;
    let hScale = factor * this.height;
    for (let j = 1; j <= this.height; j++) {
      let prevPos = j * this.rowSize - 1;
      let currentPos = j * this.rowSize;
      let nextPos = j * this.rowSize + 1;
      let prevRow = (j - 1) * this.rowSize;
      let nextRow = (j + 1) * this.rowSize;

      for (let i = 1; i <= this.width; i++) {
        currentPos += 1;
        nextPos += 1;
        prevPos += 1;
        nextRow += 1;
        prevRow += 1;
        u[currentPos] -= wScale * (p[nextPos] - p[prevPos]);
        v[currentPos] -= hScale * (p[nextRow] - p[prevRow]);
      }
    }
    this.setBnd(1, u);
    this.setBnd(indexTwo, v);
  }

  densStep(x: Float64Array, x0: Float64Array, u: Float64Array, v: Float64Array, dt: number): void {
    this.addFields(x, x0, dt);
    this.diffuse(0, x0, x);
    this.advect(0, x, x0, u, v, dt);
  }

  velStep(u: Float64Array, v: Float64Array, u0: Float64Array, v0: Float64Array, dt: number): void {
    this.addFields(u, u0, dt);
    this.addFields(v, v0, dt);
    let temp = u0;
    u0 = u;
    u = temp;
    let temp1 = v0;
    v0 = v;
    v = temp1;
    this.diffuse2(u, u0, v, v0);
    this.project(u, v, u0, v0);
    let temp2 = u0;
    u0 = u;
    u = temp2;
    let temp3 = v0;
    v0 = v;
    v = temp3;
    this.advect(1, u, u0, u0, v0, dt);
    this.advect(indexTwo, v, v0, u0, v0, dt);
    this.project(u, v, u0, v0);
  }

  queryUI(d: Float64Array, u: Float64Array, v: Float64Array): void {
    for (let i = 0; i < this.size; i++) {
      u[i] = 0.0;
      v[i] = 0.0;
      d[i] = 0.0;
    }
    prepareFrame(d, u, v);
  }

  update(): void {
    this.queryUI(this.densPrev, this.uPrev, this.vPrev);
    this.velStep(this.u, this.v, this.uPrev, this.vPrev, this.dt);
    this.densStep(this.dens, this.densPrev, this.u, this.v, this.dt);
  }

  setIterations(iters: number): void {
    if (iters > 0 && iters <= itersMax) {
      this.iterations = iters;
    }
  }

  constructor() {
    this.iterations = 10;
    this.visc = 0.5;
    this.dt = 0.1;
    this.setResolution(point, point);
  }

  iterations: number;
  visc: number;
  dt: number;
  dens: Float64Array = new Float64Array(0);
  densPrev: Float64Array = new Float64Array(0);
  u: Float64Array = new Float64Array(0);
  uPrev: Float64Array = new Float64Array(0);
  vPrev: Float64Array = new Float64Array(0);
  v: Float64Array = new Float64Array(0);
  width: number = 0;
  height: number = 0;
  rowSize: number = 0;
  size: number = 0;
  reset(): void {
    this.rowSize = this.width + indexTwo;
    this.size = (this.width + indexTwo) * (this.height + indexTwo);
    this.dens = new Float64Array(this.size);
    this.densPrev = new Float64Array(this.size);
    this.u = new Float64Array(this.size);
    this.uPrev = new Float64Array(this.size);
    this.v = new Float64Array(this.size);
    this.vPrev = new Float64Array(this.size);
    for (let i = 0; i < this.size; i++) {
      this.densPrev[i] = this.uPrev[i] = this.vPrev[i] = this.dens[i] = this.u[i] = this.v[i] = 0;
    }
  }

  setResolution(hRes: number, wRes: number): void {
    let res = wRes * hRes;
    if (res > 0 && res < pointMax && (wRes !== this.width || hRes !== this.height)) {
      this.width = wRes;
      this.height = hRes;
      this.reset();
    }
  }

  getDens(): Float64Array {
    return this.dens;
  }

  addPoints(md: Float64Array, mu: Float64Array, mv: Float64Array): void {
    let n = 64;
    for (let i = 1; i <= n; i++) {
      this.setVelocity(mu, mv, i, i, n, n);
      this.setDensity(md, i, i, densInt0);
      this.setVelocity(mu, mv, i, n - i, -n, -n);
      this.setDensity(md, i, n - i, densInt1);
      this.setVelocity(mu, mv, point2 - i, n + i, -n, -n);
      this.setDensity(md, point2 - i, n + i, densInt2);
    }
  }

  setDensity(den: Float64Array, x: number, y: number, d: number): void {
    den[x + 1 + (y + 1) * this.rowSize] = d;
  }

  setVelocity(u: Float64Array, v: Float64Array, x: number, y: number, xv: number, yv: number): void {
    u[x + 1 + (y + 1) * this.rowSize] = xv;
    v[x + 1 + (y + 1) * this.rowSize] = yv;
  }
}
/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  constructor() {
    setupNavierStokes();
  }

  runIteration(): void {
    runNavierStokes();
  }
  /*
   * @Benchmark
   */
  run(): void {
    let start = ArkTools.timeInUs() / multiple;
    for (let i = 0; i < runCount; i++) {
      this.runIteration();
    }
    let end = ArkTools.timeInUs() / multiple;
    print('navier-stokes: ms = ' + (end - start));
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().run();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().run();

declare interface ArkTools {
  timeInUs(args: number): number;
}


function log(msg: string): void {
  let debug: boolean = false;
  if (debug) {
    print(msg);
  }
}
