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
// import { BenchmarkRunner } from "../../../utils/benchmarkTsSuite";
// declare function print(arg: string, arg1?: string): string;
declare interface ArkTools {
  timeInUs(arg:any):number
}
let n = 0;
class Vec3 {
  public static set(out: Vec3, x: number, y: number, z: number): Vec3 {
    out.x = x;
    out.y = y;
    out.z = z;
    return out;
  }
  public static add(out: Vec3, a: Vec3, b: Vec3): Vec3 {
    out.x = a.x + b.x;
    out.y = a.y + b.y;
    out.z = a.z + b.z;
    return out;
  }
  public static multiplyScalar(out: Vec3, a: Vec3, b: number): Vec3 {
    out.x = a.x * b;
    out.y = a.y * b;
    out.z = a.z * b;
    return out;
  }
  public static scaleAndAdd(out: Vec3, a: Vec3, b: Vec3, scale: number): Vec3 {
    out.x = a.x + b.x * scale;
    out.y = a.y + b.y * scale;
    out.z = a.z + b.z * scale;
    return out;
  }
  public static copy(out: Vec3, a: Vec3): Vec3 {
    out.x = a.x;
    out.y = a.y;
    out.z = a.z;
    return out;
  }
  public x: number;
  public y: number;
  public z: number;
  constructor(x: number, y: number, z: number) {
    this.x = x;
    this.y = y;
    this.z = z;
  }
}
class Particle {
  public particleSystem: ParticleSystemRenderCPU;
  public position: Vec3;
  public velocity: Vec3;
  public animatedVelocity: Vec3;
  public ultimateVelocity: Vec3;
  public startSize: Vec3;
  public size: Vec3;
  public randomSeed: number;
  public remainingLifetime: number;
  public loopCount: number;
  public lastLoop: number;
  public trailDelay: number;
  public startLifetime: number;
  public emitAccumulator0: number;
  public emitAccumulator1: number;
  public frameIndex: number;
  public startRow: number;
  constructor(particleSystem: ParticleSystemRenderCPU) {
    this.particleSystem = particleSystem;
    this.position = new Vec3(0, 0, 0);
    this.velocity = new Vec3(0, 0, 0);
    this.animatedVelocity = new Vec3(0, 0, 0);
    this.ultimateVelocity = new Vec3(0, 0, 0);
    this.startSize = new Vec3(0, 0, 0);
    this.size = new Vec3(0, 0, 0);
    this.randomSeed = 0; // uint
    this.remainingLifetime = 0.2;
    this.loopCount = 0;
    this.lastLoop = 0;
    this.trailDelay = 0;
    this.startLifetime = 1;
    this.emitAccumulator0 = 0.0;
    this.emitAccumulator1 = 0.0;
    this.frameIndex = 0.0;
    this.startRow = 0;
  }
}
// let Mode: {
//   Constant: number,
//   Curve: number,
//   TwoCurves: number,
//   TwoConstants: number
// } = {
//   Constant: 0,
//   Curve: 1,
//   TwoCurves: 2,
//   TwoConstants: 3,
// };

class Mode {
  static Constant: number=0
  static Curve: number=1
  TwoCurves: number=2
  TwoConstants: number=3
}

function repeat(t: number, length: number): number {
  return t - Math.floor(t / length) * length;
}
function wrapRepeat(time: number, prevTime: number, nextTime: number): number {
  return prevTime + repeat(time - prevTime, nextTime - prevTime);
}
function binarySearchEpsilon(array: number[], value: number, EPSILON: number = 1e-6): number {
  let low = 0;
  let high = array.length - 1;
  let middle = high >>> 1;
  for (; low <= high; middle = (low + high) >>> 1) {
    n++;
    const test = array[middle];
    if (test > (value + EPSILON)) {
      high = middle - 1;
    } else if (test < (value - EPSILON)) {
      low = middle + 1;
    } else {
      return middle;
    }
  }
  return ~low;
}
function bezierInterpolate(p0: number, p1: number, p2: number, p3: number, t: number): number {
  n++;
  const u = 1 - t;
  const coeff0 = u * u * u;
  const coeff1 = 3 * u * u * t;
  const coeff2 = 3 * u * t * t;
  const coeff3 = t * t * t;
  return coeff0 * p0 + coeff1 * p1 + coeff2 * p2 + coeff3 * p3;
}
class RealKeyframeValue {
  public value: number = 0.0;
  public rightTangent: number = 0.0;
  public rightTangentWeight: number = 0.0;
  public leftTangent: number = 0.0;
  public leftTangentWeight: number = 0.0;
  constructor(a: number, b: number, c: number, d: number, e: number) {
    this.value = a;
    this.rightTangent = b;
    this.rightTangentWeight = c;
    this.leftTangent = d;
    this.leftTangentWeight = e;
  }
}
function evalBetweenTwoKeyFrames(
    prevTime: number,
    prevValue: RealKeyframeValue,
    nextTime: number,
    nextValue: RealKeyframeValue,
    ratio: number
): number {
  const dt = nextTime - prevTime;
  const ONE_THIRD = 1.0 / 3.0;
  const prevTangentWeightEnabled = false;
  const nextTangentWeightEnabled = false;
  const prevTangent = prevValue.rightTangent;
  const prevTangentWeightSpecified = prevValue.rightTangentWeight;
  // const {
  //   rightTangent: prevTangent,
  //   rightTangentWeight: prevTangentWeightSpecified,
  // } = prevValue;
  const nextTangent = nextValue.leftTangent;
  const nextTangentWeightSpecified = nextValue.leftTangentWeight
  // const {
  //   leftTangent: nextTangent,
  //   leftTangentWeight: nextTangentWeightSpecified,
  // } = nextValue;
  if (!prevTangentWeightEnabled && !nextTangentWeightEnabled) {
    const p1 = prevValue.value + ONE_THIRD * prevTangent * dt;
    const p2 = nextValue.value - ONE_THIRD * nextTangent * dt;
    return bezierInterpolate(prevValue.value, p1, p2, nextValue.value, ratio);
  }
  return 0;
}
function assertIsTrue(expr: boolean, message?: string) {
  if (!expr) {
    throw new Error(`Assertion failed:`);
  }
}
class RealCurve {
  public value: number = 0.0;
  public rightTangent: number = 0.0;
  public rightTangentWeight: number = 0.0;
  public leftTangent: number = 0.0;
  public leftTangentWeight: number = 0.0;
  protected _times: number[] = [0.1111111, 0.555555555, 0.999999999];
  protected _values: RealKeyframeValue[] = [
    new RealKeyframeValue(0.2, 0.7, 0, 0.3, 0),
    new RealKeyframeValue(0.3, 0.4, 0, 0.2, 0),
    new RealKeyframeValue(0.4, 0.5, 0, 0.6, 0)
  ];
  public evaluate(time: number): number {
    const times = this._times;
    const values = this._values;
    // const {
    //   _times: times,
    //   _values: values,
    // } = this;
    const nFrames = times.length;
    const firstTime = times[0];
    const lastTime = times[nFrames - 1];
    if (time < firstTime) {
      const preValue = values[0];
      time = wrapRepeat(time, firstTime, lastTime);
    }
    const index = binarySearchEpsilon(times, time);
    if (index >= 0) {
      return values[index].value;
    }
    const iNext = ~index;
    // assertIsTrue(iNext !== 0 && iNext !== nFrames && nFrames > 1);
    const iPre = iNext - 1;
    const preTime = times[iPre];
    const preValue = values[iPre];
    const nextTime = times[iNext];
    const nextValue = values[iNext];
    // assertIsTrue(nextTime > time && time > preTime);
    const dt = nextTime - preTime;
    const ratio = (time - preTime) / dt;
    return evalBetweenTwoKeyFrames(preTime, preValue, nextTime, nextValue, ratio);
  }
}
class CurveRange {
  public mode: number = Mode.Constant;
  public spline: RealCurve = new RealCurve();
  public constant: number = 1;
  public multiplier: number = 1;
  constructor(thismode: number) {
    this.mode = thismode
  }
  public evaluate(time: number): number {
    switch (this.mode) {
      case Mode.Constant:
        return this.constant;
      case Mode.Curve:
        return this.spline.evaluate(time) * this.multiplier;
      default:
    }
  }
}
class SizeModule {
  public size: CurveRange = new CurveRange(Mode.Curve);
  public x: CurveRange = new CurveRange(Mode.Curve);
  public y: CurveRange = new CurveRange(Mode.Curve);
  public z: CurveRange = new CurveRange(Mode.Curve);
  public animate(particle: Particle, dt: number): void {
    Vec3.multiplyScalar(particle.size, particle.startSize,
      this.size.evaluate(1 - particle.remainingLifetime / particle.startLifetime));
  }
}
class VelocityModule {
  public x: CurveRange = new CurveRange(Mode.Curve);
  public y: CurveRange = new CurveRange(Mode.Curve);
  public z: CurveRange = new CurveRange(Mode.Curve);
  public speedModifier: CurveRange = new CurveRange(Mode.Constant);
  public space: number = 0;
  private needTransform: boolean;
  public _temp_v3: Vec3 = new Vec3(0, 0, 0);
  constructor() {
    this.speedModifier.constant = 1;
    this.needTransform = false;
  }
  animate(p: Particle, dt: number): void {
    const normalizedTime = 1 - p.remainingLifetime / p.startLifetime;
    const vel = Vec3.set(this._temp_v3,
      this.x.evaluate(normalizedTime),
      this.y.evaluate(normalizedTime),
      this.z.evaluate(normalizedTime));
    if (this.needTransform) {
    }
    Vec3.add(p.animatedVelocity, p.animatedVelocity, vel);
    Vec3.add(p.ultimateVelocity, p.velocity, p.animatedVelocity);
    Vec3.multiplyScalar(p.ultimateVelocity, p.ultimateVelocity,
      this.speedModifier.evaluate(1 - p.remainingLifetime / p.startLifetime));
  }
}
class ParticleSystemRenderCPU {
  private _particles: Particle[];
  private _sizeModule: SizeModule;
  private _velocityModule: VelocityModule;
  constructor() {
    let size = 50;
    this._particles = new Array(size);
    for (let i = 0; i < size; i++) {
      this._particles[i] = new Particle(this);
    }
    this._sizeModule = new SizeModule();
    this._velocityModule = new VelocityModule();
  }
  UpdateParticles(dt: number): void {
    for (let i = 0; i < this._particles.length; ++i) {
      const p = this._particles[i];
      Vec3.set(p.animatedVelocity, 0, 0, 0);
      Vec3.copy(p.ultimateVelocity, p.velocity);
      this._sizeModule.animate(p, dt);
      this._velocityModule.animate(p, dt);
      Vec3.scaleAndAdd(p.position, p.position, p.ultimateVelocity, dt); // apply velocity.
    }
  }
}

export function RunCocos():number{
  let systems: ParticleSystemRenderCPU[] = new Array(18)
  for (let i = 0; i < 18; i++) {
    systems[i] = new ParticleSystemRenderCPU();
  }
  let start: number = ArkTools.timeInUs();
  for (let j = 0; j < 600; j++) {
    for (let i = 0; i < 18; i++) {
      systems[i].UpdateParticles(0.5);
    }
  }
  let end: number = ArkTools.timeInUs();
  let time = (end - start) / 1000
  print("Cocos - RunCocos:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunCocos()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunCocos()
// let runner1 = new BenchmarkRunner("Cocos - RunCocos", RunCocos);
// runner1.run();
