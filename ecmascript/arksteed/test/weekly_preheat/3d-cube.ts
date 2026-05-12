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

const LOOP_MAX: number = 50;
const VARRAY_LENGHT: number = 9;
const NORMAL_LENGHT: number = 6;
const HALF_NUMBER: number = 2;
const ARRAY_MAX_COUNT_FOUR: number = 4;
const ARRAY_MAX_COUNT_THR: number = 3;
const ARRAY_INDEX_TWO: number = 2;
const ARRAY_INDEX_THREE: number = 3;
const ARRAY_INDEX_FIVE: number = 5;
const ARRAY_INDEX_SIX: number = 6;
const ARRAY_INDEX_EIGHT: number = 8;
const ARRAY_INDEX_SEVEN: number = 7;
const ARRAY_INDEX_NINE: number = 9;
const ARRAY_INDEX_TEN: number = 10;
const ARRAY_INDEX_ELEVEN: number = 11;
const LINEAR_MEASURE: number = 180;
const VALIDATION_TWENTY: number = 20;
const VALIDATION_TWENTY_VALUE: number = 2889.0000000000045;
const VALIDATION_FORTY: number = 40;
const VALIDATION_FORTY_VALUE: number = 2889.0000000000055;
const VALIDATION_EIGHT: number = 80;
const VALIDATION_EIGHT_VALUE: number = 2889.000000000005;
const VALIDATION_OHS: number = 160;
const VALIDATION_OHS_VALUE: number = 2889.0000000000055;
const QARRAY_ONE_VALUE: number = 150;
const TIME_UNIT: number = 1000;
const TIME_COUNT: number = 9;
const DRAW_QUBE_COUNT: number = 5;

class Object2 {
  v: number[] = new Array();
}

class Object1 {
  loopCount: number = 0;
  loopMax: number = LOOP_MAX;
  timeMax: number = 0;
  timeAvg: number = 0;
  timeMin: number = 0;
  timeTemp: number = 0;
  timeTotal: number = 0;
  init: Boolean = false;
}

class CreateP {
  v: number[];
  constructor(x: number, y: number, z: number) {
    this.v = [x, y, x, 1];
  }
}

class Q {
  vArray: CreateP[] = new Array(VARRAY_LENGHT);
  edge: number[][] = new Array();
  normal: number[][] = new Array(NORMAL_LENGHT);
  line: boolean[] = new Array();
  numPx: number = 0;
  lastPx: number = 0;
}

class Cube {
  util(z1: number, z2: number): number {
    if (z2 > z1) {
      return 1;
    } else {
      return -1;
    }
  }

  drawLine(from: CreateP, to: CreateP): void {
    let x1 = from.v[0];
    let x2 = to.v[0];
    let y1 = from.v[1];
    let y2 = to.v[1];
    let dx = Math.abs(x2 - x1);
    let dy = Math.abs(y2 - y1);
    let x = x1;
    let y = y1;
    let incX1: number;
    let incY1: number;
    let incX2: number;
    let incY2: number;
    let den: number;
    let num: number;
    let numAdd: number;
    let numPix: number;

    incX1 = this.util(x1, x2);
    incX2 = incX1;
    incY1 = this.util(y1, y2);
    incY2 = incY1;

    if (dx >= dy) {
      incX1 = 0;
      incY2 = 0;
      den = dx;
      num = dx / HALF_NUMBER;
      numAdd = dy;
      numPix = dx;
    } else {
      incX2 = 0;
      incY1 = 0;
      den = dy;
      num = dy / HALF_NUMBER;
      numAdd = dx;
      numPix = dy;
    }
    numPix = this.q.lastPx + numPix;

    for (let i = this.q.lastPx; i < numPix; i++) {
      num += numAdd;
      if (num >= den) {
        x += incX1;
        y += incY1;
      }
      x += incX2;
      y += incY2;
    }
    this.q.lastPx = numPix;
  }

  calcCross(v0: number[], v1: number[]): number[] {
    let cross: number[] = [0, 0, 0, 0];
    cross[0] = v0[1] * v1[ARRAY_INDEX_TWO] - v0[ARRAY_INDEX_TWO] * v1[1];
    cross[1] = v0[ARRAY_INDEX_TWO] * v1[0] - v0[0] * v1[ARRAY_INDEX_TWO];
    cross[ARRAY_INDEX_TWO] = v0[0] * v1[1] - v0[1] * v1[0];
    return cross;
  }

  calcNormal(v0: number[], v1: number[], v2: number[]): number[] {
    let a: number[] = [0, 0, 0, 0];
    let b: number[] = [0, 0, 0, 0];
    for (let i = 0; i < ARRAY_MAX_COUNT_THR; i++) {
      a[i] = v0[i] - v1[i];
      b[i] = v2[i] - v1[i];
    }
    a = this.calcCross(a, b);
    let x = a[0] * a[0] + a[1] * a[1] + a[ARRAY_INDEX_TWO] * a[ARRAY_INDEX_TWO];
    let length = Math.sqrt(x);
    for (let i = 0; i < ARRAY_MAX_COUNT_THR; i++) {
      a[i] = a[i] / length;
    }
    a[ARRAY_INDEX_THREE] = 1;
    return a;
  }

  mMulti(m1: number[][], m2: number[][]): number[][] {
    let m: number[][] = [
      [0, 0, 0, 0],
      [0, 0, 0, 0],
      [0, 0, 0, 0],
      [0, 0, 0, 0]
    ];
    for (let i = 0; i < ARRAY_MAX_COUNT_FOUR; i++) {
      for (let j = 0; j < ARRAY_MAX_COUNT_FOUR; j++) {
        m[i][j] =
          m1[i][0] * m2[0][j] + m1[i][1] * m2[1][j] + m1[i][ARRAY_INDEX_TWO] * m2[ARRAY_INDEX_TWO][j] + m1[i][ARRAY_INDEX_THREE] * m2[ARRAY_INDEX_THREE][j];
      }
    }
    return m;
  }

  vMulti(m: number[][], v: number[]): number[] {
    let vect: number[] = [0, 0, 0, 0];
    for (let i = 0; i < ARRAY_MAX_COUNT_FOUR; i++) {
      vect[i] = m[i][0] * v[0] + m[i][1] * v[1] + m[i][ARRAY_INDEX_TWO] * v[ARRAY_INDEX_TWO] + m[i][ARRAY_INDEX_THREE] * v[ARRAY_INDEX_THREE];
    }
    return vect;
  }

  vMulti2(m: number[][], v: number[]): number[] {
    let vect: number[] = [0, 0, 0, 0];
    for (let i = 0; i < ARRAY_MAX_COUNT_FOUR; i++) {
      vect[i] = m[i][0] * v[0] + m[i][1] * v[1] + m[i][ARRAY_INDEX_TWO] * v[ARRAY_INDEX_TWO];
    }
    return vect;
  }

  translate(m: number[][], dx: number, dy: number, dz: number): number[][] {
    let t = [
      [1, 0, 0, dx],
      [0, 1, 0, dy],
      [0, 0, 1, dz],
      [0, 0, 0, 1]
    ];
    return this.mMulti(t, m);
  }

  rotateX(m: number[][], phi: number): number[][] {
    let a = phi;
    a *= Math.PI / LINEAR_MEASURE;
    let cos = Math.cos(a);
    let sin = Math.sin(a);
    let r: number[][] = [
      [1, 0, 0, 0],
      [0, cos, -sin, 0],
      [0, sin, cos, 0],
      [0, 0, 0, 1]
    ];
    return this.mMulti(r, m);
  }

  rotateY(m: number[][], phi: number): number[][] {
    let a = phi;
    a *= Math.PI / LINEAR_MEASURE;
    let cos = Math.cos(a);
    let sin = Math.sin(a);
    let r: number[][] = [
      [cos, 0, sin, 0],
      [0, 1, 0, 0],
      [-sin, 0, cos, 0],
      [0, 0, 0, 1]
    ];
    return this.mMulti(r, m);
  }

  rotateZ(m: number[][], phi: number): number[][] {
    let a = phi;
    a *= Math.PI / LINEAR_MEASURE;
    let cos = Math.cos(a);
    let sin = Math.sin(a);
    let r = [
      [cos, -sin, 0, 0],
      [sin, cos, 0, 0],
      [0, 0, 1, 0],
      [0, 0, 0, 1]
    ];
    return this.mMulti(r, m);
  }

  drawLine1(curN: number[][]): void {
    if (curN[0][ARRAY_INDEX_TWO] < 0) {
      if (!this.q.line[0]) {
        this.drawLine(this.q.vArray[0], this.q.vArray[1]);
        this.q.line[0] = true;
      }
      if (!this.q.line[1]) {
        this.drawLine(this.q.vArray[1], this.q.vArray[ARRAY_INDEX_TWO]);
        this.q.line[1] = true;
      }
      if (!this.q.line[ARRAY_INDEX_TWO]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_TWO], this.q.vArray[ARRAY_INDEX_TWO]);
        this.q.line[ARRAY_INDEX_TWO] = true;
      }
      if (!this.q.line[ARRAY_INDEX_TWO]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_THREE], this.q.vArray[0]);
        this.q.line[ARRAY_INDEX_TWO] = true;
      }
    }

    if (curN[1][ARRAY_INDEX_TWO] < 0) {
      if (!this.q.line[ARRAY_INDEX_TWO]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_THREE], this.q.vArray[ARRAY_INDEX_TWO]);
        this.q.line[ARRAY_INDEX_TWO] = true;
      }
      if (!this.q.line[ARRAY_INDEX_NINE]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_THREE], this.q.vArray[ARRAY_INDEX_SIX]);
        this.q.line[ARRAY_INDEX_NINE] = true;
      }
      if (!this.q.line[ARRAY_INDEX_SIX]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_SIX], this.q.vArray[ARRAY_INDEX_SEVEN]);
        this.q.line[ARRAY_INDEX_SIX] = true;
      }
      if (!this.q.line[ARRAY_INDEX_TEN]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_SEVEN], this.q.vArray[ARRAY_INDEX_THREE]);
        this.q.line[ARRAY_INDEX_TEN] = true;
      }
    }
  }

  drawLine2(curN: number[][]): void {
    if (curN[ARRAY_INDEX_THREE][ARRAY_INDEX_THREE] < 0) {
      if (!this.q.line[ARRAY_MAX_COUNT_FOUR]) {
        this.drawLine(this.q.vArray[ARRAY_MAX_COUNT_FOUR], this.q.vArray[ARRAY_INDEX_FIVE]);
        this.q.line[ARRAY_MAX_COUNT_FOUR] = true;
      }
      if (!this.q.line[ARRAY_INDEX_FIVE]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_FIVE], this.q.vArray[ARRAY_INDEX_SIX]);
        this.q.line[ARRAY_INDEX_FIVE] = true;
      }
      if (!this.q.line[ARRAY_INDEX_SIX]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_SIX], this.q.vArray[ARRAY_INDEX_SEVEN]);
        this.q.line[ARRAY_INDEX_SIX] = true;
      }
      if (!this.q.line[ARRAY_INDEX_SEVEN]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_SEVEN], this.q.vArray[ARRAY_MAX_COUNT_FOUR]);
        this.q.line[ARRAY_INDEX_SEVEN] = true;
      }
    }
    if (curN[ARRAY_INDEX_THREE][ARRAY_INDEX_THREE] < 0) {
      if (!this.q.line[ARRAY_MAX_COUNT_FOUR]) {
        this.drawLine(this.q.vArray[ARRAY_MAX_COUNT_FOUR], this.q.vArray[ARRAY_INDEX_FIVE]);
        this.q.line[ARRAY_MAX_COUNT_FOUR] = true;
      }
      if (!this.q.line[ARRAY_INDEX_EIGHT]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_FIVE], this.q.vArray[1]);
        this.q.line[ARRAY_INDEX_EIGHT] = true;
      }
      if (!this.q.line[0]) {
        this.drawLine(this.q.vArray[1], this.q.vArray[0]);
        this.q.line[0] = true;
      }
      if (!this.q.line[ARRAY_INDEX_ELEVEN]) {
        this.drawLine(this.q.vArray[0], this.q.vArray[ARRAY_MAX_COUNT_FOUR]);
        this.q.line[ARRAY_INDEX_ELEVEN] = true;
      }
    }
  }

  drawQube(): void {
    let curN: number[][] = new Array();
    this.q.lastPx = 0;
    for (let i = DRAW_QUBE_COUNT; i >= 0; i--) {
      curN.push(this.vMulti2(this.mQube, this.q.normal[i]));
    }
    this.drawLine1(curN);
    this.drawLine2(curN);
    if (curN[ARRAY_MAX_COUNT_FOUR][ARRAY_INDEX_TWO] < 0) {
      if (!this.q.line[ARRAY_INDEX_ELEVEN]) {
        this.drawLine(this.q.vArray[ARRAY_MAX_COUNT_FOUR], this.q.vArray[0]);
        this.q.line[ARRAY_INDEX_ELEVEN] = true;
      }
      if (!this.q.line[ARRAY_INDEX_THREE]) {
        this.drawLine(this.q.vArray[0], this.q.vArray[ARRAY_INDEX_THREE]);
        this.q.line[ARRAY_INDEX_THREE] = true;
      }
      if (!this.q.line[ARRAY_INDEX_TEN]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_THREE], this.q.vArray[ARRAY_INDEX_SEVEN]);
        this.q.line[ARRAY_INDEX_TEN] = true;
      }
      if (!this.q.line[ARRAY_INDEX_SEVEN]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_SEVEN], this.q.vArray[ARRAY_MAX_COUNT_FOUR]);
        this.q.line[ARRAY_INDEX_SEVEN] = true;
      }
    }
    if (curN[ARRAY_INDEX_FIVE][ARRAY_INDEX_TWO] < 0) {
      if (!this.q.line[ARRAY_INDEX_EIGHT]) {
        this.drawLine(this.q.vArray[1], this.q.vArray[ARRAY_INDEX_FIVE]);
        this.q.line[ARRAY_INDEX_EIGHT] = true;
      }
      if (!this.q.line[ARRAY_INDEX_FIVE]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_FIVE], this.q.vArray[ARRAY_INDEX_SIX]);
        this.q.line[ARRAY_INDEX_FIVE] = true;
      }
      if (!this.q.line[ARRAY_INDEX_NINE]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_SIX], this.q.vArray[ARRAY_INDEX_THREE]);
        this.q.line[ARRAY_INDEX_NINE] = true;
      }
      if (!this.q.line[1]) {
        this.drawLine(this.q.vArray[ARRAY_INDEX_THREE], this.q.vArray[1]);
        this.q.line[1] = true;
      }
    }
    this.q.line = [false, false, false, false, false, false, false, false, false, false, false, false];
    this.q.lastPx = 0;
  }

  loop(): void {
    if (this.testing.loopCount > this.testing.loopMax) {
      return;
    }
    let testingStr = String(this.testing.loopCount);
    while (testingStr.length < ARRAY_MAX_COUNT_THR) {
      testingStr = '0' + testingStr;
    }
    this.mTrans = this.translate(
      this.i,
      -this.q.vArray[ARRAY_INDEX_EIGHT].v[0],
      -this.q.vArray[ARRAY_INDEX_EIGHT].v[1],
      -this.q.vArray[ARRAY_INDEX_EIGHT].v[ARRAY_INDEX_TWO]
    );
    this.mTrans = this.rotateX(this.mTrans, 1);
    this.mTrans = this.rotateY(this.mTrans, ARRAY_MAX_COUNT_THR);
    this.mTrans = this.rotateZ(this.mTrans, ARRAY_INDEX_FIVE);
    this.mTrans = this.translate(
      this.mTrans,
      this.q.vArray[ARRAY_INDEX_EIGHT].v[0],
      this.q.vArray[ARRAY_INDEX_EIGHT].v[1],
      this.q.vArray[ARRAY_INDEX_EIGHT].v[ARRAY_INDEX_TWO]
    );
    this.mQube = this.mMulti(this.mTrans, this.mQube);
    for (let i = 8; i >= 0; i--) {
      this.q.vArray[i].v = this.vMulti(this.mTrans, this.q.vArray[i].v);
    }
    this.drawQube();
    this.testing.loopCount += 1;
    this.loop();
  }

  q = new Q();
  // transformation matrix
  mTrans: number[][] = new Array();
  // position information of qube
  mQube: number[][] = new Array();
  // entity matrix
  i: number[][] = new Array();
  origin = new Object2();
  testing = new Object1();
  validation = new Map<number, number>([
    [VALIDATION_TWENTY, VALIDATION_TWENTY_VALUE],
    [VALIDATION_FORTY, VALIDATION_FORTY_VALUE],
    [VALIDATION_EIGHT, VALIDATION_EIGHT_VALUE],
    [VALIDATION_OHS, VALIDATION_OHS_VALUE]
  ]);

  create1(cubeSize: number): void {
    // init/reset vars
    this.origin.v = [QARRAY_ONE_VALUE, QARRAY_ONE_VALUE, VALIDATION_TWENTY, 1];
    this.testing.loopCount = 0;
    this.testing.loopMax = 50;
    this.testing.timeMax = 0;
    this.testing.timeAvg = 0;
    this.testing.timeMin = 0;
    this.testing.timeTemp = 0;
    this.testing.timeTotal = 0;
    this.testing.init = false;
    // transformation matrix
    this.mTrans = [
      [1, 0, 0, 0],
      [0, 1, 0, 0],
      [0, 0, 1, 0],
      [0, 0, 0, 1]
    ];

    // position information of qube
    this.mQube = [
      [1, 0, 0, 0],
      [0, 1, 0, 0],
      [0, 0, 1, 0],
      [0, 0, 0, 1]
    ];

    // entity matrix
    this.i = [
      [1, 0, 0, 0],
      [0, 1, 0, 0],
      [0, 0, 1, 0],
      [0, 0, 0, 1]
    ];

    // create qube
    this.q.vArray[0] = new CreateP(-cubeSize, -cubeSize, cubeSize);
    this.q.vArray[1] = new CreateP(-cubeSize, cubeSize, cubeSize);
    this.q.vArray[ARRAY_INDEX_TWO] = new CreateP(cubeSize, cubeSize, cubeSize);
    this.q.vArray[ARRAY_INDEX_THREE] = new CreateP(cubeSize, -cubeSize, cubeSize);
    this.q.vArray[ARRAY_MAX_COUNT_FOUR] = new CreateP(-cubeSize, -cubeSize, -cubeSize);
    this.q.vArray[ARRAY_INDEX_FIVE] = new CreateP(-cubeSize, cubeSize, -cubeSize);
    this.q.vArray[ARRAY_INDEX_SIX] = new CreateP(cubeSize, cubeSize, -cubeSize);
    this.q.vArray[ARRAY_INDEX_SEVEN] = new CreateP(cubeSize, -cubeSize, -cubeSize);
    this.q.vArray[ARRAY_INDEX_EIGHT] = new CreateP(0, 0, 0);
  }

  create2(cubeSize: number): void {
    // anti-clockwise edge check
    this.q.edge = [
      [0, 1, ARRAY_INDEX_TWO],
      [ARRAY_INDEX_THREE, ARRAY_INDEX_TWO, ARRAY_INDEX_SIX],
      [ARRAY_INDEX_SEVEN, ARRAY_INDEX_SIX, ARRAY_INDEX_FIVE],
      [ARRAY_MAX_COUNT_FOUR, ARRAY_INDEX_FIVE, 1],
      [ARRAY_MAX_COUNT_FOUR, 0, ARRAY_INDEX_THREE],
      [1, ARRAY_INDEX_FIVE, ARRAY_INDEX_SIX]
    ];

    for (let i = 0; i < this.q.edge.length; i++) {
      this.q.normal[i] = this.calcNormal(
        this.q.vArray[this.q.edge[i][0]].v,
        this.q.vArray[this.q.edge[i][1]].v,
        this.q.vArray[this.q.edge[i][ARRAY_INDEX_TWO]].v
      );
    }
    this.q.line = [false, false, false, false, false, false, false, false, false, false, false, false];
    this.q.numPx = ARRAY_INDEX_NINE * ARRAY_INDEX_TWO * cubeSize;
    for (let i = 0; i < this.q.numPx; i++) {
      new CreateP(0, 0, 0);
    }

    this.mTrans = this.translate(this.mTrans, this.origin.v[0], this.origin.v[1], this.origin.v[ARRAY_INDEX_TWO]);
    this.mQube = this.mMulti(this.mTrans, this.mQube);

    for (let i = 0; i < TIME_COUNT; i++) {
      this.q.vArray[i].v = this.vMulti(this.mTrans, this.q.vArray[i].v);
    }

    this.drawQube();
    this.testing.init = true;
    this.loop();
  }

  creat(cubeSize: number): void {
    this.create1(cubeSize);
    this.create2(cubeSize);

    // debugLog(
    //   'cubeSize is' +
    //     cubeSize +
    //     'this.MTrans value is' +
    //     this.mTrans[0][0] +
    //     this.mTrans[0][1] +
    //     this.mTrans[0][ARRAY_INDEX_TWO] +
    //     this.mTrans[0][ARRAY_INDEX_THREE]
    // );
    // debugLog(
    //   'cubeSize is' +
    //     cubeSize +
    //     'this.MQube value is' +r
    //     this.mQube[0][0] +
    //     this.mQube[0][1] +
    //     this.mQube[0][ARRAY_INDEX_TWO] +
    //     this.mQube[0][ARRAY_INDEX_THREE]
    // );

    // Perform a simple sum-based verification.
    let sum: number = 0;
    for (let i = 0; i < this.q.vArray.length; i++) {
      let vector = this.q.vArray[i].v;
      for (let j = 0; j < vector.length; j++) {
        sum += vector[j];
      }
    }
    if (sum !== this.validation.get(cubeSize)) {
      //debugLog("Error: bad vector sum for cubeSize = " + cubeSize + "; expected " + this.validation.get(cubeSize) + " but got " + sum)
    }
  }

  runTest(): void {
    let i = VALIDATION_TWENTY;
    while (i <= VALIDATION_OHS) {
      this.creat(i);
      i *= ARRAY_INDEX_TWO;
    }
  }
}

let isDebug: boolean = false;
function debugLog(msg: string): void {
  if (isDebug) {
    print(msg);
  }
}

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  /*
   * @Benchmark
   */
  runIterationTime(): void {
    let start = ArkTools.timeInUs() / TIME_UNIT;
    let cube = new Cube();
    for (let i = 0; i < ARRAY_INDEX_EIGHT; i++) {
      cube.runTest();
    }
    let end = ArkTools.timeInUs() / TIME_UNIT;
    print('3d-cube: ms = ' + (end - start));
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
