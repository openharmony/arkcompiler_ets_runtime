
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

const BLUR_NUM_0_5: number = 0.5;
const BLUR_NUM_1000: number = 1000;
const BLUR_NUM_120: number = 120;
const BLUR_NUM_0_726: number = 0.726;
const BLUR_NUM_NEGATIVE_2: number = -2;
const BLUR_NUM_2: number = 2;
const BLUR_NUM_8: number = 8;
const BLUR_NUM_16: number = 16;
const BLUR_NUM_24: number = 24;
const BLUR_NUM_6: number = 6;
const BLUR_NUM_4: number = 4;
const BLUR_NUM_5: number = 5;
const BLUR_NUM_3: number = 3;
const BLUR_NUM_7: number = 7;
const BLUR_NUM_12: number = 12;
const BLUR_NUM_19: number = 19;
const BLUR_NUM_9: number = 9;
const HEX_FF: number = 0xff;
const HEX_7ED55D16: number = 0x7ed55d16;
const HEX_C761C23C: number = 0xc761c23c;
const HEX_FFFFFFFF: number = 0xffffffff;
const HEX_FFFFFFF: number = 0xfffffff;
const HEX_165667B1: number = 0x165667b1;
const HEX_D3A2646C: number = 0xd3a2646c;
const HEX_FD7046C5: number = 0xfd7046c5;
const HEX_B55A4F09: number = 0xb55a4f09;
const HEX_10000000: number = 0x10000000;
const BLUR_ONE_HUNDRED_TWENTY: number = 20;

class GaussianBlur {
  // Configuration.
  a0: number = 0;
  a1: number = 0;
  a2: number = 0;
  a3: number = 0;
  b1: number = 0;
  b2: number = 0;
  leftCorner: number = 0;
  lrightCorner: number = 0;

  static gaussCoef(sigma: number): Float32Array {
    //debugLog("gaussCoef=====sigma", sigma)
    if (sigma < BLUR_NUM_0_5) {
      sigma = BLUR_NUM_0_5;
    }

    const a = Math.exp(BLUR_NUM_0_726 * BLUR_NUM_0_726) / sigma;
    const g1 = Math.exp(-a);
    const g2 = Math.exp(BLUR_NUM_NEGATIVE_2 * a);
    const k = ((1 - g1) * (1 - g1)) / (1 + BLUR_NUM_2 * a * g1 - g2);
    let a0 = k;
    let a1 = k * (a - 1) * g1;
    let a2 = k * (a + 1) * g1;
    let a3 = -k * g2;
    let b1 = BLUR_NUM_2 * g1;
    let b2 = -g2;
    let leftCorner = (a0 + a1) / (1 - b1 - b2);
    let lrightCorner = (a2 + a3) / (1 - b1 - b2);
    //debugLog("gaussCoef=====([a0, a1, a2, a3, b1, b2, leftCorner, lrightCorner])",  ([a0, a1, a2, a3, b1, b2, leftCorner, lrightCorner]))
    return new Float32Array([a0, a1, a2, a3, b1, b2, leftCorner, lrightCorner]);
  }

  static convolveRGBA(src: Uint32Array, out: Uint32Array, line: Float32Array, coeff: number[] | Float32Array, width: number, height: number): void {
    // takes src image and writes the blurred and transposed result into out
    //debugLog("convolveRGBA=====src length", src.length)
    //debugLog("convolveRGBA=====out length", out.length)
    //debugLog("convolveRGBA=====line length",line.length)
    //debugLog("convolveRGBA=====coeff", coeff.length)
    //debugLog("convolveRGBA=====width", width)
    //debugLog("convolveRGBA=====height", height)
    let rgb: number;
    let preSrcR: number;
    let preCrcG: number;
    let preSrcB: number;
    let preSrcA: number;
    let currSrcR: number;
    let currSrcG: number;
    let currSrcB: number;
    let currSrcA: number;
    let currOutR: number;
    let currOutG: number;
    let currOutB: number;
    let currOutA: number;
    let preOutR: number;
    let preOutG: number;
    let preOutB: number;
    let preOutA: number;
    let prepreOutR: number;
    let prepreOutG: number;
    let prepreOutB: number;
    let prepreOutA: number;
    let srcIndex: number;
    let outIndex: number;
    let lineIndex: number;
    let coeffA0: number;
    let coeffA1: number;
    let coeffB1: number;
    let coeffB2: number;
    let i: number;
    let j: number;
    for (i = 0; i < height; i++) {
      srcIndex = i * width;
      outIndex = i;
      lineIndex = 0;

      // left to right;
      rgb = src[srcIndex];

      preSrcR = rgb & HEX_FF;
      preCrcG = (rgb >> BLUR_NUM_8) & HEX_FF;
      preSrcB = (rgb >> BLUR_NUM_16) & HEX_FF;
      preSrcA = (rgb >> BLUR_NUM_24) & HEX_FF;

      prepreOutR = preSrcR * coeff[BLUR_NUM_6];
      prepreOutG = preCrcG * coeff[BLUR_NUM_6];
      prepreOutB = preSrcB * coeff[BLUR_NUM_6];
      prepreOutA = preSrcA * coeff[BLUR_NUM_6];

      preOutR = prepreOutR;
      preOutG = prepreOutG;
      preOutB = prepreOutB;
      preOutA = prepreOutA;

      coeffA0 = coeff[0];
      coeffA1 = coeff[1];
      coeffB1 = coeff[BLUR_NUM_4];
      coeffB2 = coeff[BLUR_NUM_5];

      for (j = 0; j < width; j++) {
        rgb = src[srcIndex];
        currSrcR = rgb & HEX_FF;
        currSrcG = (rgb >> BLUR_NUM_8) & HEX_FF;
        currSrcB = (rgb >> BLUR_NUM_16) & HEX_FF;
        currSrcA = (rgb >> BLUR_NUM_24) & HEX_FF;

        currOutR = currSrcR * coeffA0 + preSrcR * coeffA1 + preOutR * coeffB1 + prepreOutR * coeffB2;
        currOutG = currSrcG * coeffA0 + preCrcG * coeffA1 + preOutG * coeffB1 + prepreOutG * coeffB2;
        currOutB = currSrcB * coeffA0 + preSrcB * coeffA1 + preOutB * coeffB1 + prepreOutB * coeffB2;
        currOutA = currSrcA * coeffA0 + preSrcA * coeffA1 + preOutA * coeffB1 + prepreOutA * coeffB2;

        prepreOutR = preOutR;
        prepreOutG = preOutG;
        prepreOutB = preOutB;
        prepreOutA = preOutA;

        preOutR = currOutR;
        preOutG = currOutG;
        preOutB = currOutB;
        preOutA = currOutA;

        preSrcR = currSrcR;
        preCrcG = currSrcG;
        preSrcB = currSrcB;
        preSrcA = currSrcA;

        line[lineIndex] = preOutR;
        line[lineIndex + 1] = preOutG;
        line[lineIndex + BLUR_NUM_2] = preOutB;
        line[lineIndex + BLUR_NUM_3] = preOutA;
        lineIndex += BLUR_NUM_4;
        srcIndex++;
      }
      srcIndex--;
      lineIndex -= BLUR_NUM_4;
      outIndex += height * (width - 1);

      // right to left
      rgb = src[srcIndex];

      preSrcR = rgb & HEX_FF;
      preCrcG = (rgb >> BLUR_NUM_8) & HEX_FF;
      preSrcB = (rgb >> BLUR_NUM_16) & HEX_FF;
      preSrcA = (rgb >> BLUR_NUM_24) & HEX_FF;

      prepreOutR = preSrcR * coeff[BLUR_NUM_7];
      prepreOutG = preCrcG * coeff[BLUR_NUM_7];
      prepreOutB = preSrcB * coeff[BLUR_NUM_7];
      prepreOutA = preSrcA * coeff[BLUR_NUM_7];

      preOutR = prepreOutR;
      preOutG = prepreOutG;
      preOutB = prepreOutB;
      preOutA = prepreOutA;

      currSrcR = preSrcR;
      currSrcG = preCrcG;
      currSrcB = preSrcB;
      currSrcA = preSrcA;

      coeffA0 = coeff[BLUR_NUM_2];
      coeffA1 = coeff[BLUR_NUM_3];

      for (j = width - 1; j >= 0; j--) {
        currOutR = currSrcR * coeffA0 + preSrcR * coeffA1 + preOutR * coeffB1 + prepreOutR * coeffB2;
        currOutG = currSrcG * coeffA0 + preCrcG * coeffA1 + preOutG * coeffB1 + prepreOutG * coeffB2;
        currOutB = currSrcB * coeffA0 + preSrcB * coeffA1 + preOutB * coeffB1 + prepreOutB * coeffB2;
        currOutA = currSrcA * coeffA0 + preSrcA * coeffA1 + preOutA * coeffB1 + prepreOutA * coeffB2;

        prepreOutR = preOutR;
        prepreOutG = preOutG;
        prepreOutB = preOutB;
        prepreOutA = preOutA;

        preOutR = currOutR;
        preOutG = currOutG;
        preOutB = currOutB;
        preOutA = currOutA;

        preSrcR = currSrcR;
        preCrcG = currSrcG;
        preSrcB = currSrcB;
        preSrcA = currSrcA;

        rgb = src[srcIndex];
        currSrcR = rgb & HEX_FF;
        currSrcG = (rgb >> BLUR_NUM_8) & HEX_FF;
        currSrcB = (rgb >> BLUR_NUM_16) & HEX_FF;
        currSrcA = (rgb >> BLUR_NUM_24) & HEX_FF;

        rgb =
          ((line[lineIndex] + preOutR) << 0) +
          ((line[lineIndex + 1] + preOutG) << BLUR_NUM_8) +
          ((line[lineIndex + BLUR_NUM_2] + preOutB) << BLUR_NUM_16) +
          ((line[lineIndex + BLUR_NUM_3] + preOutA) << BLUR_NUM_24);

        out[outIndex] = rgb;

        srcIndex--;
        lineIndex -= BLUR_NUM_4;
        outIndex -= height;
      }
    }
    //debugLog("out length", out.length)
  }
  static blurRGBA(src: Uint32Array, width: number, height: number, radius: number): void {
    //debugLog("convolveRGBA=====src length",src.length)
    //debugLog("convolveRGBA=====width", width)
    //debugLog("convolveRGBA=====height",height)
    //debugLog("convolveRGBA=====radius",radius)
    // Quick exit on zero radius
    if (!radius) {
      return;
    }

    // Unify input data type, to keep convolver calls isomorphic
    const src32 = new Uint32Array(src.buffer);
    const out = new Uint32Array(src32.length);
    const tmpLine = new Float32Array(Math.max(width, height) * BLUR_NUM_4);
    const coeff = GaussianBlur.gaussCoef(radius);

    GaussianBlur.convolveRGBA(src32, out, tmpLine, coeff, width, height);
    GaussianBlur.convolveRGBA(out, src32, tmpLine, coeff, height, width);
  }
}
/*
 *  @State
 *  @Tags Jetstream2
 */
class Benchmark {
  width: number;
  height: number;
  radius: number;
  buffer: Uint32Array;
  /*
   * @Setup
   */
  constructor() {
    this.width = 800;
    this.height = 450;
    this.radius = 15;
    const rand = ((): (() => {}) => {
      let seedNum: number = 49734321;
      return (): number => {
        seedNum = (seedNum + HEX_7ED55D16 + (seedNum << BLUR_NUM_12)) & HEX_FFFFFFFF;
        seedNum = (seedNum ^ HEX_C761C23C ^ (seedNum >>> BLUR_NUM_19)) & HEX_FFFFFFFF;
        seedNum = (seedNum + HEX_165667B1 + (seedNum << BLUR_NUM_5)) & HEX_FFFFFFFF;
        seedNum = ((seedNum + HEX_D3A2646C) ^ (seedNum << BLUR_NUM_9)) & HEX_FFFFFFFF;
        seedNum = (seedNum + HEX_FD7046C5 + (seedNum << BLUR_NUM_3)) & HEX_FFFFFFFF;
        seedNum = (seedNum ^ HEX_B55A4F09 ^ (seedNum >>> BLUR_NUM_16)) & HEX_FFFFFFFF;
        return (seedNum & HEX_FFFFFFF) / HEX_10000000;
      };
    })();

    const bufferArry = new Uint32Array(this.width * this.height);
    for (let i = 0; i < bufferArry.length; ++i) {
      bufferArry[i] = rand();
    }
    this.buffer = bufferArry;
  }
  /*
   * @Benchmark
   */
  runIteration(): void {
    let startTime = ArkTools.timeInUs() / BLUR_NUM_1000;
    for (let i = 0; i < BLUR_ONE_HUNDRED_TWENTY; i++) {
      GaussianBlur.blurRGBA(this.buffer, this.width, this.height, this.radius);
    }
    let endTime = ArkTools.timeInUs() / BLUR_NUM_1000;
    print(`gaussian-blur: ms = ${endTime - startTime}`);
  }
}

declare interface ArkTools {
  timeInUs(args: number): number;
}

function debugLog(str: String, object: number | number[]): void {
  let isLog = false;
  if (isLog) {
    print(`${str}: ${object}`);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().runIteration();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().runIteration();
