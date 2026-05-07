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
import { B2DistanceProxy, B2SimplexCache, B2DistanceInput, B2DistanceOutput, b2DistanceM } from './b2Distance';
import { B2Vec2, subtract, B2Sweep, b2Cross12, b2MulTR2, b2MulT2, b2Dot22, b2MulR2, add, multM, minus } from '../Common/b2Math';
import { b2LinearSlop, b2MaxPolygonVertices, float025, half, float3, indexTwo, floopCount } from '../Common/b2Settings';

export class B2ToiInput {
  constructor() {}
  public proxyA = new B2DistanceProxy();
  public proxyB = new B2DistanceProxy();
  public sweepA = new B2Sweep();
  public sweepB = new B2Sweep();
  public tMax: number = 0.0;
}

export enum State {
  UNKNOWN,
  FAILED,
  OVERLAPPED,
  TOUCHING,
  SEPARATED
}

export class B2ToiOutput {
  constructor() {}
  public state = State.UNKNOWN;
  public t: number = 0;
}

export function b2TimeOfImpact(output: B2ToiOutput, input: B2ToiInput): void {
  output.state = State.UNKNOWN;
  output.t = input.tMax;
  let proxyA = input.proxyA;
  let proxyB = input.proxyB;
  let sweepA = input.sweepA;
  let sweepB = input.sweepB;
  sweepA.normalize();
  sweepB.normalize();
  let tMax = input.tMax;
  let totalRadius = proxyA.mRadius + proxyB.mRadius;
  let target = Math.max(b2LinearSlop, totalRadius - float3 * b2LinearSlop);
  let tolerance = float025 * b2LinearSlop;
  let t1: number = 0.0;
  let kMaxIterations = 20;
  let iter = 0;
  let cache = new B2SimplexCache();
  cache.count = 0;
  let distanceInput = new B2DistanceInput();
  distanceInput.proxyA = input.proxyA;
  distanceInput.proxyB = input.proxyB;
  distanceInput.useRadii = false;
  while (true) {
    let xfA = sweepA.getTransform(t1);
    let xfB = sweepB.getTransform(t1);
    distanceInput.transformA = xfA;
    distanceInput.transformB = xfB;
    let distanceOutput = new B2DistanceOutput();
    b2DistanceM(distanceOutput, cache, distanceInput);
    if (distanceOutput.distance <= 0.0) {
      output.state = State.OVERLAPPED;
      output.t = 0.0;
      break;
    }
    if (distanceOutput.distance < target + tolerance) {
      output.state = State.TOUCHING;
      output.t = t1;
      break;
    }
    let fcn = new B2SeparationFunction();
    fcn.initialize(cache, proxyA, sweepA, proxyB, sweepB, t1);
    let done = false;
    let t2 = tMax;
    let pushBackIter = 0;
    while (true) {
      let fms = fcn.findMinSeparation(t2);
      let s2 = fms[0];
      let indexA = fms[1];
      let indexB = fms[indexTwo];
      if (s2 > target + tolerance) {
        output.state = State.SEPARATED;
        output.t = tMax;
        done = true;
        break;
      }
      if (s2 > target - tolerance) {
        t1 = t2;
        break;
      }
      let s1 = fcn.evaluate(indexA, indexB, t1);
      if (s1 < target - tolerance) {
        output.state = State.FAILED;
        output.t = t1;
        done = true;
        break;
      }
      if (s1 <= target + tolerance) {
        output.state = State.TOUCHING;
        output.t = t1;
        done = true;
        break;
      }
      let rootIterCount = 0;
      let a1 = t1;
      let a2 = t2;
      while (true) {
        let t: number;
        if ((rootIterCount & 1) !== 0) {
          t = a1 + ((target - s1) * (a2 - a1)) / (s2 - s1);
        } else {
          t = half * (a1 + a2);
        }
        rootIterCount += 1;
        let s = fcn.evaluate(indexA, indexB, t);
        if (Math.abs(s - target) < tolerance) {
          t2 = t;
          break;
        }
        if (s > target) {
          a1 = t;
          s1 = s;
        } else {
          a2 = t;
          s2 = s;
        }
        if (rootIterCount === floopCount) {
          break;
        }
      }
      b2ToiMaxRootIters = Math.max(b2ToiMaxRootIters, rootIterCount);
      pushBackIter += 1;
      if (pushBackIter === b2MaxPolygonVertices) {
        break;
      }
    }
    iter += 1;
    if (done) {
      break;
    }
    if (iter === kMaxIterations) {
      output.state = State.FAILED;
      output.t = t1;
      break;
    }
  }
  b2ToiMaxIters = Math.max(b2ToiMaxIters, iter);
}
let b2ToiMaxIters = 0;
let b2ToiMaxRootIters = 0;

enum TYPE {
  POINTS,
  FACEA,
  FACEB
}

export class B2SeparationFunction {
  initialize(cache: B2SimplexCache, proxyA: B2DistanceProxy, sweepA: B2Sweep, proxyB: B2DistanceProxy, sweepB: B2Sweep, t1: number): number {
    this.mProxyA = proxyA;
    this.mProxyB = proxyB;
    let count = cache.count;
    this.mSweepA = sweepA;
    this.mSweepB = sweepB;
    let xfA = this.mSweepA.getTransform(t1);
    let xfB = this.mSweepB.getTransform(t1);
    if (count === 1) {
      this.mType = TYPE.POINTS;
      let localPointA = this.mProxyA.getVertex(cache.indexA[0]);
      let localPointB = this.mProxyB.getVertex(cache.indexB[0]);
      let pointA = b2MulT2(xfA, localPointA);
      let pointB = b2MulT2(xfB, localPointB);
      this.mAxis = subtract(pointB, pointA);
      let s = this.mAxis.normalize();
      return s;
    } else if (cache.indexA[0] === cache.indexA[1]) {
      this.mType = TYPE.FACEB;
      let localPointB1 = proxyB.getVertex(cache.indexB[0]);
      let localPointB2 = proxyB.getVertex(cache.indexB[1]);
      this.mAxis = b2Cross12(subtract(localPointB2, localPointB1), 1.0);
      this.mAxis.normalize();
      let normal = b2MulR2(xfB.q, this.mAxis);
      this.mLocalPoint = multM(add(localPointB1, localPointB2), half);
      let pointB = b2MulT2(xfB, this.mLocalPoint);
      let localPointA = proxyA.getVertex(cache.indexA[0]);
      let pointA = b2MulT2(xfA, localPointA);
      let s = b2Dot22(subtract(pointA, pointB), normal);
      if (s < 0.0) {
        this.mAxis = minus(this.mAxis);
        s = -s;
      }
      return s;
    } else {
      this.mType = TYPE.FACEA;
      let localPointA1 = this.mProxyA.getVertex(cache.indexA[0]);
      let localPointA2 = this.mProxyA.getVertex(cache.indexA[1]);
      this.mAxis = b2Cross12(subtract(localPointA2, localPointA1), 1.0);
      this.mAxis.normalize();
      let normal = b2MulR2(xfA.q, this.mAxis);
      this.mLocalPoint = multM(add(localPointA1, localPointA2), half);
      let pointA = b2MulT2(xfA, this.mLocalPoint);
      let localPointB = this.mProxyB.getVertex(cache.indexB[0]);
      let pointB = b2MulT2(xfB, localPointB);
      let s = b2Dot22(subtract(pointB, pointA), normal);
      if (s < 0.0) {
        this.mAxis = minus(this.mAxis);
        s = -s;
      }
      return s;
    }
  }

  findMinSeparation(t: number): [separation: number, indexA: number, indexB: number] {
    let indexA: number;
    let indexB: number;
    let xfA = this.mSweepA.getTransform(t);
    let xfB = this.mSweepB.getTransform(t);
    switch (this.mType) {
      case TYPE.POINTS: {
        let axisA1 = b2MulTR2(xfA.q, this.mAxis);
        let axisB1 = b2MulTR2(xfB.q, minus(this.mAxis));
        indexA = this.mProxyA.getSupport(axisA1);
        indexB = this.mProxyB.getSupport(axisB1);
        let localPointA1 = this.mProxyA.getVertex(indexA);
        let localPointB1 = this.mProxyB.getVertex(indexB);
        let pointA1 = b2MulT2(xfA, localPointA1);
        let pointB1 = b2MulT2(xfB, localPointB1);
        let separation1 = b2Dot22(subtract(pointB1, pointA1), this.mAxis);
        return [separation1, indexA, indexB];
      }
      case TYPE.FACEA: {
        let normal2 = b2MulR2(xfA.q, this.mAxis);
        let pointA2 = b2MulT2(xfA, this.mLocalPoint);
        let axisB = b2MulTR2(xfB.q, minus(normal2));
        indexA = -1;
        indexB = this.mProxyB.getSupport(axisB);
        let localPointB = this.mProxyB.getVertex(indexB);
        let pointB2 = b2MulT2(xfB, localPointB);
        let separation2 = b2Dot22(subtract(pointB2, pointA2), normal2);
        return [separation2, indexA, indexB];
      }
      case TYPE.FACEB: {
        let normal = b2MulR2(xfB.q, this.mAxis);
        let pointB = b2MulT2(xfB, this.mLocalPoint);
        let axisA = b2MulTR2(xfA.q, minus(normal));
        indexB = -1;
        indexA = this.mProxyA.getSupport(axisA);
        let localPointA = this.mProxyA.getVertex(indexA);
        let pointA = b2MulT2(xfA, localPointA);
        let separation = b2Dot22(subtract(pointA, pointB), normal);
        return [separation, indexA, indexB];
      }
      default:
        break;
    }
    return [0, 0, 0];
  }

  evaluate(indexA: number, indexB: number, t: number): number {
    let xfA = this.mSweepA.getTransform(t);
    let xfB = this.mSweepB.getTransform(t);
    switch (this.mType) {
      case TYPE.POINTS: {
        let localPointA1 = this.mProxyA.getVertex(indexA);
        let localPointB1 = this.mProxyB.getVertex(indexB);
        let pointA1 = b2MulT2(xfA, localPointA1);
        let pointB1 = b2MulT2(xfB, localPointB1);
        let separation1 = b2Dot22(subtract(pointB1, pointA1), this.mAxis);
        return separation1;
      }
      case TYPE.FACEA: {
        let normal2 = b2MulR2(xfA.q, this.mAxis);
        let pointA2 = b2MulT2(xfA, this.mLocalPoint);
        let localPointB = this.mProxyB.getVertex(indexB);
        let pointB2 = b2MulT2(xfB, localPointB);
        let separation2 = b2Dot22(subtract(pointB2, pointA2), normal2);
        return separation2;
      }
      case TYPE.FACEB: {
        let normal = b2MulR2(xfB.q, this.mAxis);
        let pointB = b2MulT2(xfB, this.mLocalPoint);
        let localPointA = this.mProxyA.getVertex(indexA);
        let pointA = b2MulT2(xfA, localPointA);
        let separation = b2Dot22(subtract(pointA, pointB), normal);
        return separation;
      }
      default:
        break;
    }
    return 0;
  }

  mProxyA = new B2DistanceProxy();
  mProxyB = new B2DistanceProxy();
  mSweepA = new B2Sweep();
  mSweepB = new B2Sweep();
  mType = TYPE.POINTS;
  mLocalPoint = new B2Vec2();
  mAxis = new B2Vec2();
}
