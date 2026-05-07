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
import { int4, b2Epsilon, b2PI, float2, int16, indexTwo, b2MaxSubSteps } from './b2Settings';

function b2IsValid(x: number): boolean {
  return Number.isNaN(x) === false && Number.isFinite(x) === false;
}

function b2Sqrt(x: number): number {
  return Math.sqrt(x);
}

function b2Atan2(y: number, x: number): number {
  return Math.atan2(y, x);
}

export class B2Vec2 {
  public x: number;
  public y: number;
  constructor(x0: number = 0, y0: number = 0) {
    if (x0 !== null && y0 !== null) {
      this.x = x0;
      this.y = y0;
    } else {
      this.x = 0;
      this.y = 0;
    }
  }

  public setZero(): void {
    this.x = 0.0;
    this.y = 0.0;
  }
  
  public set(x0: number, y0: number): void {
    this.x = x0;
    this.y = y0;
  }
  getSubscript(index: number): number {
    if (index === 0) {
      return this.x;
    } else {
      return this.y;
    }
  }
  setSubscript(index: number, newValue: number): void {
    if (index === 0) {
      this.x = newValue;
    } else {
      this.y = newValue;
    }
  }
  public length(): number {
    return Math.sqrt(this.x * this.x + this.y * this.y);
  }
  public lengthSquared(): number {
    return this.x * this.x + this.y * this.y;
  }
  public normalize(): number {
    let length = this.length();
    if (length < b2Epsilon) {
      return 0.0;
    }
    let invLength = 1.0 / length;
    this.x *= invLength;
    this.y *= invLength;
    return length;
  }
  public isValid(): boolean {
    return b2IsValid(this.x) && b2IsValid(this.y);
  }
  public skew(): B2Vec2 {
    return new B2Vec2(-this.y, this.x);
  }
}

export function minus(v: B2Vec2): B2Vec2 {
  let mv = new B2Vec2(-v.x, -v.y);
  return mv;
}

export function addEqual(a: B2Vec2, b: B2Vec2): void {
  a.x += b.x;
  a.y += b.y;
}

export function subtractEqual(a: B2Vec2, b: B2Vec2): void {
  a.x -= b.x;
  a.y -= b.y;
}

export function mulEqual(a: B2Vec2, b: number): void {
  a.x *= b;
  a.y *= b;
}

export class B2Vec3 {
  x: number;
  y: number;
  z: number;
  constructor(x1: number = 0, y1: number = 0, z1: number = 0) {
    if (x1 !== null && y1 !== null && z1 !== null) {
      this.x = x1;
      this.y = y1;
      this.z = z1;
    } else {
      this.x = 0.0;
      this.y = 0.0;
      this.z = 0.0;
    }
  }
  
  setZero(): void {
    this.x = 0.0;
    this.y = 0.0;
  }

  set(x1: number, y1: number, z1: number): void {
    this.x = x1;
    this.y = y1;
    this.z = z1;
  }
}

export function minus3(v: B2Vec3): B2Vec3 {
  let mv = new B2Vec3(-v.x, -v.y, -v.z);
  return mv;
}

export function addEqual3(a: B2Vec3, b: B2Vec3): void {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
}

export function mulMEqual3(a: B2Vec3, b: number): void {
  a.x *= b;
  a.y *= b;
  a.z *= b;
}

export class B2Mat22 {
  public ex: B2Vec2;
  public ey: B2Vec2;
  public setMat(angle: number): void {
    let c = Math.cos(angle);
    let s = Math.sin(angle);
    this.ex.set(c, s);
    this.ey.set(-s, c);
  }
  
  constructor(c1: B2Vec2 | null = null, c2: B2Vec2 | null = null) {
    if (c1 !== null && c2 !== null) {
      this.ex = c1;
      this.ey = c2;
    } else {
      this.ex = new B2Vec2(0.0, 0.0);
      this.ey = new B2Vec2(0.0, 0.0);
    }
  }
  
  public set(c1: B2Vec2, c2: B2Vec2): void {
    this.ex = c1;
    this.ey = c2;
  }
  public setIdentity(): void {
    this.ex.x = 1.0;
    this.ey.x = 0.0;
    this.ex.y = 0.0;
    this.ey.y = 1.0;
  }
  public setZero(): void {
    this.ex.x = 0.0;
    this.ey.x = 0.0;
    this.ex.y = 0.0;
    this.ey.y = 0.0;
  }
  public getInverse(): B2Mat22 {
    let a = this.ex.x;
    let b = this.ey.x;
    let c = this.ex.y;
    let d = this.ey.y;
    let mB = new B2Mat22();
    let det = a * d - b * c;
    if (det !== 0.0) {
      det = 1.0 / det;
    }
    mB.ex.x = det * d;
    mB.ey.x = -det * b;
    mB.ex.y = -det * c;
    mB.ey.y = det * a;
    return mB;
  }
  public solve(b: B2Vec2): B2Vec2 {
    let a11 = this.ex.x;
    let a12 = this.ey.x;
    let a21 = this.ex.y;
    let a22 = this.ey.y;
    let det = a11 * a22 - a12 * a21;
    if (det !== 0.0) {
      det = 1.0 / det;
    }
    let x = new B2Vec2();
    x.x = det * (a22 * b.x - a12 * b.y);
    x.y = det * (a11 * b.y - a21 * b.x);
    return x;
  }
}

export class B2Mat33 {
  public ex: B2Vec3;
  public ey: B2Vec3;
  public ez: B2Vec3;
  constructor(c1: B2Vec3 | null = null, c2: B2Vec3 | null = null, c3: B2Vec3 | null = null) {
    if (c1 !== null && c2 !== null && c3 !== null) {
      this.ex = c1;
      this.ey = c2;
      this.ez = c3;
    } else {
      this.ex = new B2Vec3();
      this.ey = new B2Vec3();
      this.ez = new B2Vec3();
    }
  }
  public setZero(): void {
    this.ex.setZero();
    this.ey.setZero();
    this.ez.setZero();
  }
  public solve33(b: B2Vec3): B2Vec3 {
    let det = b2Dot33(this.ex, b2Cross33(this.ey, this.ez));
    if (det !== 0.0) {
      det = 1.0 / det;
    }
    let x = new B2Vec3();
    x.x = det * b2Dot33(b, b2Cross33(this.ey, this.ez));
    x.y = det * b2Dot33(this.ex, b2Cross33(b, this.ez));
    x.z = det * b2Dot33(this.ex, b2Cross33(this.ey, b));
    return x;
  }
  public solve22(b: B2Vec2): B2Vec2 {
    let a11 = this.ex.x;
    let a12 = this.ey.x;
    let a21 = this.ex.y;
    let a22 = this.ey.y;
    let det = a11 * a22 - a12 * a21;
    if (det !== 0.0) {
      det = 1.0 / det;
    }
    let x = new B2Vec2();
    x.x = det * (a22 * b.x - a12 * b.y);
    x.y = det * (a11 * b.y - a21 * b.x);
    return x;
  }
  public getInverse22(): B2Mat33 {
    let a = this.ex.x;
    let b = this.ey.x;
    let c = this.ex.y;
    let d = this.ey.y;
    let det = a * d - b * c;
    if (det !== 0.0) {
      det = 1.0 / det;
    }
    let mM = new B2Mat33();
    mM.ex.x = det * d;
    mM.ey.x = -det * b;
    mM.ex.z = 0.0;
    mM.ex.y = -det * c;
    mM.ey.y = det * a;
    mM.ey.z = 0.0;
    mM.ez.x = 0.0;
    mM.ez.y = 0.0;
    mM.ez.z = 0.0;
    return mM;
  }
  
  public getSymInverse33(): B2Mat33 {
    let det = b2Dot33(this.ex, b2Cross33(this.ey, this.ez));
    if (det !== 0.0) {
      det = 1.0 / det;
    }
    let a11 = this.ex.x;
    let a12 = this.ey.x;
    let a13 = this.ez.x;
    let a22 = this.ey.y;
    let a23 = this.ez.y;
    let a33 = this.ez.z;
    let mM = new B2Mat33();
    mM.ex.x = det * (a22 * a33 - a23 * a23);
    mM.ex.y = det * (a13 * a23 - a12 * a33);
    mM.ex.z = det * (a12 * a23 - a13 * a22);
    mM.ey.x = mM.ex.y;
    mM.ey.y = det * (a11 * a33 - a13 * a13);
    mM.ey.z = det * (a13 * a12 - a11 * a23);
    mM.ez.x = mM.ex.z;
    mM.ez.y = mM.ey.z;
    mM.ez.z = det * (a11 * a22 - a12 * a12);
    return mM;
  }
}

export class B2Rot {
  public s: number;
  public c: number;
  constructor(angle: number | null = null) {
    if (angle !== null) {
      this.s = Math.sin(angle);
      this.c = Math.cos(angle);
    } else {
      this.s = 0.0;
      this.c = 0.0;
    }
  }
  
  public set(angle: number): void {
    this.s = Math.sin(angle);
    this.c = Math.cos(angle);
  }
  public setIdentity(): void {
    this.s = 0.0;
    this.c = 1.0;
  }
  public angle(): number {
    return b2Atan2(this.s, this.c);
  }
  public xAxis(): B2Vec2 {
    return new B2Vec2(this.c, this.s);
  }
  public yAxis(): B2Vec2 {
    return new B2Vec2(-this.s, this.c);
  }
}

export class B2Transform {
  public p: B2Vec2;
  public q: B2Rot;
  constructor(position: B2Vec2 | null = null, rotation: B2Rot | null = null) {
    if (position !== null && rotation !== null) {
      this.p = position;
      this.q = rotation;
    } else {
      this.p = new B2Vec2();
      this.q = new B2Rot();
    }
  }
  public setIdentity(): void {
    this.p.setZero();
    this.q.setIdentity();
  }
  public set(position: B2Vec2, angle: number): void {
    this.p = position;
    this.q.set(angle);
  }
}

export class B2Sweep {
  public localCenter = new B2Vec2();
  public mc0 = new B2Vec2();
  get c0(): B2Vec2 {
    return this.mc0;
  }
  set c0(value: B2Vec2) {
    this.mc0 = value;
  }
  public c = new B2Vec2();
  public a0: number = 0;
  public a: number = 0;
  public alpha0: number = 0;
  constructor() {}
  public getTransform(beta: number): B2Transform {
    let xf = new B2Transform();
    xf.p = add(multM(this.c0, 1.0 - beta), multM(this.c, beta));
    let angle = (1.0 - beta) * this.a0 + beta * this.a;
    xf.q.set(angle);
    subtractEqual(xf.p, b2MulR2(xf.q, this.localCenter));
    return xf;
  }
  public advance(alpha: number): void {
    let beta = (alpha - this.alpha0) / (1.0 - this.alpha0);
    addEqual(this.c0, multM(subtract(this.c, this.c0), beta));
    this.a0 += beta * (this.a - this.a0);
    this.alpha0 = alpha;
  }
  public normalize(): void {
    let twoPi = float2 * b2PI;
    let d = twoPi * Math.floor(this.a0 / twoPi);
    this.a0 -= d;
    this.a -= d;
  }
}

let b2Vec2Zero = new B2Vec2(0.0, 0.0);
function b2Dot22(a: B2Vec2, b: B2Vec2): number {
  return a.x * b.x + a.y * b.y;
}

function b2Cross(a: B2Vec2, b: B2Vec2): number {
  return a.x * b.y - a.y * b.x;
}

function b2Cross12(a: B2Vec2, s: number): B2Vec2 {
  return new B2Vec2(s * a.y, -s * a.x);
}

function b2Cross21(s: number, a: B2Vec2): B2Vec2 {
  return new B2Vec2(-s * a.y, s * a.x);
}

function b2Mul22(a: B2Mat22, v: B2Vec2): B2Vec2 {
  return new B2Vec2(b2Dot22(v, a.ex), b2Dot22(v, a.ey));
}

function b2MulTM2(a: B2Mat22, v: B2Vec2): B2Vec2 {
  return new B2Vec2(b2Dot22(v, a.ex), b2Dot22(v, a.ey));
}

function add(a: B2Vec2, b: B2Vec2): B2Vec2 {
  return new B2Vec2(a.x + b.x, a.y + b.y);
}

function subtract(a: B2Vec2, b: B2Vec2): B2Vec2 {
  return new B2Vec2(a.x - b.x, a.y - b.y);
}

function multM(a: B2Vec2, b: number): B2Vec2 {
  return new B2Vec2(a.x * b, a.y * b);
}

function b2Distance(a: B2Vec2, b: B2Vec2): number {
  let c = new B2Vec2(a.x - b.x, a.y - b.y);
  return c.length();
}

function b2DistanceSquared(a: B2Vec2, b: B2Vec2): number {
  let c = new B2Vec2(a.x - b.x, a.y - b.y);
  return b2Dot22(c, c);
}

function multM3(s: number, a: B2Vec3): B2Vec3 {
  return new B2Vec3(s * a.x, s * a.y, s * a.z);
}

function add3(a: B2Vec3, b: B2Vec3): B2Vec3 {
  return new B2Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

function subtract3(a: B2Vec3, b: B2Vec3): B2Vec3 {
  return new B2Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

function b2Dot33(a: B2Vec3, b: B2Vec3): number {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

function b2Cross33(a: B2Vec3, b: B2Vec3): B2Vec3 {
  return new B2Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

function b2Mul(a: B2Mat22, b: B2Mat22): B2Mat22 {
  return new B2Mat22(b2Mul22(a, b.ex), b2Mul22(a, b.ey));
}

function b2Mul33(a: B2Mat33, v: B2Vec3): B2Vec3 {
  return add3(add3(multM3(v.x, a.ex), multM3(v.y, a.ey)), multM3(v.z, a.ez));
}

function b2Mul32(a: B2Mat33, v: B2Vec2): B2Vec2 {
  return new B2Vec2(a.ex.x * v.x + a.ey.x * v.y, a.ex.y * v.x + a.ey.y * v.y);
}

function b2MulTRR(q: B2Rot, r: B2Rot): B2Rot {
  let qr = new B2Rot();
  qr.s = q.c * r.s - q.s * r.c;
  qr.c = q.c * r.c + q.s * r.s;
  return qr;
}

function b2MulR2(q: B2Rot, v: B2Vec2): B2Vec2 {
  return new B2Vec2(q.c * v.x - q.s * v.y, q.s * v.x + q.c * v.y);
}

function b2MulTR2(q: B2Rot, v: B2Vec2): B2Vec2 {
  return new B2Vec2(q.c * v.x + q.s * v.y, -q.s * v.x + q.c * v.y);
}

function b2MulT2(t: B2Transform, v: B2Vec2): B2Vec2 {
  let x = t.q.c * v.x - t.q.s * v.y + t.p.x;
  let y = t.q.s * v.x + t.q.c * v.y + t.p.y;
  return new B2Vec2(x, y);
}

function b2MulTT2(t: B2Transform, v: B2Vec2): B2Vec2 {
  let px = v.x - t.p.x;
  let py = v.y - t.p.y;
  let x = t.q.c * px + t.q.s * py;
  let y = -t.q.s * px + t.q.c * py;
  return new B2Vec2(x, y);
}

function b2MulT(a: B2Transform, b: B2Transform): B2Transform {
  let mC = new B2Transform();
  mC.q = b2MulTRR(a.q, b.q);
  let ma = new B2Vec2(b.p.x - a.p.x, b.p.y - a.p.y);
  mC.p = b2MulTR2(a.q, ma);
  return mC;
}

function b2Abs2(a: B2Vec2): B2Vec2 {
  return new B2Vec2(Math.abs(a.x), Math.abs(a.y));
}

function b2Min(a: B2Vec2, b: B2Vec2): B2Vec2 {
  return new B2Vec2(Math.min(a.x, b.x), Math.min(a.y, b.y));
}

function b2Max(a: B2Vec2, b: B2Vec2): B2Vec2 {
  return new B2Vec2(Math.max(a.x, b.x), Math.max(a.y, b.y));
}

function b2ClampF(a: number, low: number, high: number): number {
  return Math.max(low, Math.min(a, high));
}

export function b2NextPowerOfTwo(x0: number): number {
  let x = x0;
  x = x | (x >> 1);
  x = x | (x >> indexTwo);
  x = x | (x >> int4);
  x = x | (x >> b2MaxSubSteps);
  x = x | (x >> int16);
  return x + 1;
}

export { b2MulT, add, add3, b2Mul32, b2Mul33, subtract, multM, b2MulTT2, multM3, subtract3, b2MulTM2 };
export { b2Vec2Zero, b2DistanceSquared, b2Min, b2Max, b2Abs2, b2ClampF, b2Mul22, b2Atan2 };
export { b2Mul, b2MulR2, b2Dot22, b2Sqrt, b2Cross, b2MulT2, b2Distance, b2MulTR2, b2Cross21, b2Cross12 };
