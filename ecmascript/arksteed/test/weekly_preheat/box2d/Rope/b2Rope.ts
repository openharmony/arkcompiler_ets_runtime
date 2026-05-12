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
import { B2Vec2, b2Atan2, b2Cross, b2Dot22, b2Distance, subtract, multM, addEqual, mulEqual, subtractEqual, minus } from '../Common/b2Math';
import { b2PI, float2, indexTwo } from '../Common/b2Settings';
class B2RopeDef {
  vertices: B2Vec2[] | null;
  count: number;
  masses: number[] | null;
  gravity: B2Vec2 = new B2Vec2();
  damping: number;
  k2: number;
  k3: number;
  constructor() {
    this.vertices = null;
    this.count = 0;
    this.masses = null;
    this.gravity = new B2Vec2();
    this.damping = 0.1;
    this.k2 = 0.9;
    this.k3 = 0.1;
  }
}

export class B2Rope {
  mCount: number;
  mps: B2Vec2[] | null = null;
  mp0s: B2Vec2[] | null = null;
  mvs: B2Vec2[] | null = null;
  mims: number[] | null = null;
  mLs: number[] | null = null;
  mas: number[] | null = null;
  mGravity: B2Vec2;
  mDamping: number;
  mk2: number;
  mk3: number;
  constructor() {
    this.mCount = 0;
    this.mps = [];
    this.mp0s = [];
    this.mvs = [];
    this.mims = [];
    this.mLs = [];
    this.mas = [];
    this.mGravity = new B2Vec2(0.0, 0.0);
    this.mDamping = 0.1;
    this.mk2 = 1.0;
    this.mk3 = 0.1;
  }

  initialize(def: B2RopeDef): void {
    this.mCount = def.count;
    this.mps = [];
    this.mp0s = [];
    this.mvs = [];
    this.mims = [];
    for (let i = 0; i < this.mCount; i++) {
      this.mps.push(new B2Vec2());
      this.mp0s.push(new B2Vec2());
      this.mvs.push(new B2Vec2());
      this.mims.push(0.0);
    }
    for (let i = 0; i < this.mCount; i++) {
      this.mps[i] = def.vertices![i];
      this.mp0s[i] = def.vertices![i];
      this.mvs[i].setZero();
      let m = def.masses![i];
      if (m > 0.0) {
        this.mims[i] = 1.0 / m;
      } else {
        this.mims[i] = 0.0;
      }
    }
    let count2 = this.mCount - 1;
    let count3 = this.mCount - indexTwo;
    this.mLs = [];
    for (let i = 0; i < count2; i++) {
      this.mLs.push(0.0);
    }
    this.mas = [];
    for (let j = 0; j < count3; j++) {
      this.mas.push(0.0);
    }
    for (let i = 0; i < count2; i++) {
      let p1 = this.mps[i];
      let p2 = this.mps[i + 1];
      this.mLs[i] = b2Distance(p1, p2);
    }
    for (let i = 0; i < count3; i++) {
      let p1 = this.mps[i];
      let p2 = this.mps[i + 1];
      let p3 = this.mps[i + indexTwo];
      let d1 = subtract(p2, p1);
      let d2 = subtract(p3, p2);
      let a = b2Cross(d1, d2);
      let b = b2Dot22(d1, d2);
      this.mas[i] = b2Atan2(a, b);
    }
    this.mGravity = def.gravity;
    this.mDamping = def.damping;
    this.mk2 = def.k2;
    this.mk3 = def.k3;
  }

  step(h: number, iterations: number): void {
    if (h === 0.0) {
      return;
    }
    let d = Math.exp(-h * this.mDamping);
    for (let i = 0; i < this.mCount; i++) {
      this.mp0s![i] = this.mps![i];
      if (this.mims![i] > 0.0) {
        addEqual(this.mvs![i], multM(this.mGravity, h));
      }
      mulEqual(this.mvs![i], d);
      addEqual(this.mps![i], multM(this.mvs![i], h));
    }
    for (let i = 0; i < iterations; i++) {
      this.solveC2();
      this.solveC3();
      this.solveC2();
    }
    let invh = 1.0 / h;
    for (let i = 0; i < this.mCount; i++) {
      this.mvs![i] = multM(subtract(this.mps![i], this.mp0s![i]), invh);
    }
  }

  get vertexCount(): number {
    return this.mCount;
  }

  get vertices(): B2Vec2[] {
    return this.mps!;
  }

  setAngle(angle: number): void {
    let count3 = this.mCount - indexTwo;
    for (let i = 0; i < count3; i++) {
      this.mas![i] = angle;
    }
  }

  solveC2(): void {
    let count2 = this.mCount - 1;
    for (let i = 0; i < count2; i++) {
      let p1 = this.mps![i];
      let p2 = this.mps![i + 1];
      let d = subtract(p2, p1);
      let L = d.normalize();
      let im1 = this.mims![i];
      let im2 = this.mims![i + 1];
      if (im1 + im2 === 0.0) {
        continue;
      }
      let s1 = im1 / (im1 + im2);
      let s2 = im2 / (im1 + im2);
      subtractEqual(p1, multM(d, this.mk2 * s1 * (this.mLs![i] - L)));
      addEqual(p2, multM(d, this.mk2 * s2 * (this.mLs![i] - L)));
      this.mps![i] = p1;
      this.mps![i + 1] = p2;
    }
  }

  solveC3(): void {
    let count3 = this.mCount - indexTwo;
    for (let i = 0; i < count3; i++) {
      let p1 = this.mps![i];
      let p2 = this.mps![i + 1];
      let p3 = this.mps![i + indexTwo];
      let m1 = this.mims![i];
      let m2 = this.mims![i + 1];
      let m3 = this.mims![i + indexTwo];
      let d1 = subtract(p2, p1);
      let d2 = subtract(p3, p2);
      let l1sqr = d1.lengthSquared();
      let l2sqr = d2.lengthSquared();
      if (l1sqr * l2sqr === 0.0) {
        continue;
      }
      let a = b2Cross(d1, d2);
      let b = b2Dot22(d1, d2);
      let angle = b2Atan2(a, b);
      let jd1 = multM(d1.skew(), -1.0 / l1sqr);
      let jd2 = multM(d2.skew(), 1.0 / l2sqr);
      let j1 = minus(jd1);
      let j2 = subtract(jd1, jd2);
      let j3 = jd2;
      let mass = m1 * b2Dot22(j1, j1) + m2 * b2Dot22(j2, j2) + m3 * b2Dot22(j3, j3);
      if (mass === 0.0) {
        continue;
      }
      mass = 1.0 / mass;
      let c = angle - this.mas![i];
      while (c > b2PI) {
        angle -= float2 * b2PI;
        c = angle - this.mas![i];
      }
      while (c < -b2PI) {
        angle += float2 * b2PI;
        c = angle - this.mas![i];
      }
      let impulse = -this.mk3 * mass * c;
      addEqual(p1, multM(j1, m1 * impulse));
      addEqual(p2, multM(j2, m2 * impulse));
      addEqual(p3, multM(j3, m3 * impulse));
      this.mps![i] = p1;
      this.mps![i + 1] = p2;
      this.mps![i + indexTwo] = p3;
    }
  }
}
