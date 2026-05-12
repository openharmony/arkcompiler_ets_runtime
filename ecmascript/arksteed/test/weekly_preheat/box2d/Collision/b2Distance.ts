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
import {
  B2Vec2,
  B2Transform,
  b2Dot22,
  subtractEqual,
  b2Cross,
  b2Distance,
  b2Vec2Zero,
  b2Cross21,
  b2Cross12,
  b2MulT2,
  b2MulTR2,
  multM,
  add,
  subtract,
  addEqual,
  minus
} from '../Common/b2Math';
import { B2Shape, B2CircleShape, B2PolygonShape, B2EdgeShape } from './Shapes/b2Shape';
import { b2Epsilon, b2MaxFloat, b2MaxManifoldPoints, indexThr, indexTwo } from '../Common/b2Settings';
import { B2Array } from '../Dynamics/b2TimeStep';
import { float2 } from '../Common/b2Settings';
import { half } from '../Common/b2Settings';

export class B2DistanceProxy {
  public mBuffer: B2Array<B2Vec2> = new B2Array(float2, new B2Vec2());
  public mVertices: B2Array<B2Vec2> = new B2Array(1, new B2Vec2());
  public mCount = 0;
  public mRadius: number = 0;
  public constructor(shape: B2Shape | null = null) {
    if (shape !== null) {
      this.set(shape);
    }
  }

  public set(shape: B2Shape): void {
    switch (shape.type.name) {
      case 'CIRCLE': {
        let circle = shape as B2CircleShape;
        this.mVertices = circle.mp;
        this.mCount = 1;
        this.mRadius = circle.mRadius;
        break;
      }
      case 'POLYGON': {
        let polygon = shape as B2PolygonShape;
        this.mVertices = polygon.mVertices;
        this.mCount = polygon.mCount;
        this.mRadius = polygon.mRadius;
        break;
      }
      case 'EDGE': {
        let edge = shape as B2EdgeShape;
        this.mVertices = edge.mVertices;
        this.mCount = b2MaxManifoldPoints;
        this.mRadius = edge.mRadius;
        break;
      }
      default:
        break;
    }
  }

  public getSupport(d: B2Vec2): number {
    let bestIndex = 0;
    let bestValue = b2Dot22(this.mVertices.get(0), d);
    for (let i = 1; i < this.mCount; i++) {
      let value = b2Dot22(this.mVertices!.get(i), d);
      if (value > bestValue) {
        bestIndex = i;
        bestValue = value;
      }
    }
    return bestIndex;
  }

  public getSupportVertex(d: B2Vec2): B2Vec2 {
    let bestIndex = 0;
    let bestValue = b2Dot22(this.mVertices.get(0), d);
    for (let i = 1; i < this.mCount; i++) {
      let value = b2Dot22(this.mVertices!.get(i), d);
      if (value > bestValue) {
        bestIndex = i;
        bestValue = value;
      }
    }
    return this.mVertices!.get(bestIndex);
  }

  public get getVertexCount(): number {
    return this.mCount;
  }

  public getVertex(index: number): B2Vec2 {
    return this.mVertices.get(index);
  }
}

export class B2SimplexVertex {
  wA = new B2Vec2();
  wB = new B2Vec2();
  w = new B2Vec2();
  a: number = 0;
  indexA: number = 0;
  indexB: number = 0;
  set(other: B2SimplexVertex): void {
    this.wA = other.wA;
    this.wB = other.wB;
    this.w = other.w;
    this.a = other.a;
    this.indexA = other.indexA;
    this.indexB = other.indexB;
  }
}

export class B2Simplex {
  mCount: number = 0;
  constructor() {
    for (let i = 0; i < indexThr; i++) {
      this.mv.push(new B2SimplexVertex());
    }
  }

  readCache(cache: B2SimplexCache, proxyA: B2DistanceProxy, transformA: B2Transform, proxyB: B2DistanceProxy, transformB: B2Transform): void {
    this.mCount = cache.count;
    for (let i = 0; i < this.mCount; i++) {
      let v = this.mv[i];
      v.indexA = cache.indexA[i];
      v.indexB = cache.indexB[i];
      let wALocal = proxyA.getVertex(v.indexA);
      let wBLocal = proxyB.getVertex(v.indexB);
      v.wA = b2MulT2(transformA, wALocal);
      v.wB = b2MulT2(transformB, wBLocal);
      v.w = subtract(v.wB, v.wA);
      v.a = 0.0;
    }
    if (this.mCount > 1) {
      let metric1 = cache.metric;
      let metric2 = this.getMetric();
      if (metric2 < half * metric1 || float2 * metric1 < metric2 || metric2 < b2Epsilon) {
        this.mCount = 0;
      }
    }
    if (this.mCount === 0) {
      let v = this.mv[0];
      v.indexA = 0;
      v.indexB = 0;
      let wALocal = proxyA.getVertex(0);
      let wBLocal = proxyB.getVertex(0);
      v.wA = b2MulT2(transformA, wALocal);
      v.wB = b2MulT2(transformB, wBLocal);
      v.w = subtract(v.wB, v.wA);
      v.a = 1.0;
      this.mCount = 1;
    }
  }

  writeCache(cache: B2SimplexCache): void {
    cache.metric = this.getMetric();
    cache.count = this.mCount;
    for (let i = 0; i < this.mCount; i++) {
      cache.indexA[i] = this.mv[i].indexA;
      cache.indexB[i] = this.mv[i].indexB;
    }
  }

  getSearchDirection(): B2Vec2 {
    switch (this.mCount) {
      case 1:
        return minus(this.mv1.w);
      case indexTwo: {
        let e12 = subtract(this.mv2.w, this.mv1.w);
        let sgn = b2Cross(e12, minus(this.mv1.w));
        if (sgn > 0.0) {
          return b2Cross21(1.0, e12);
        } else {
          return b2Cross12(e12, 1.0);
        }
      }
      default:
        return b2Vec2Zero;
    }
  }

  getClosestPoint(): B2Vec2 {
    switch (this.mCount) {
      case 0:
        return b2Vec2Zero;
      case 1:
        return this.mv1.w;
      case indexTwo:
        return add(multM(this.mv1.w, this.mv1.a), multM(this.mv2.w, this.mv2.a));
      case indexThr:
        return b2Vec2Zero;
      default:
        return b2Vec2Zero;
    }
  }

  getWitnessPoints(): [B2Vec2, B2Vec2] {
    let pA = new B2Vec2();
    let pB = new B2Vec2();
    switch (this.mCount) {
      case 0:
        throw new Error('illegal state');
      case 1:
        pA = this.mv1.wA;
        pB = this.mv1.wB;
        break;
      case indexTwo:
        pA = add(multM(this.mv1.wA, this.mv1.a), multM(this.mv2.wA, this.mv2.a));
        pB = add(multM(this.mv1.wB, this.mv1.a), multM(this.mv2.wB, this.mv2.a));
        break;
      case indexThr:
        pA = add(add(multM(this.mv1.wA, this.mv1.a), multM(this.mv2.wA, this.mv2.a)), multM(this.mv3.wA, this.mv3.a));
        pB = pA;
        break;
      default:
        throw new Error('illegal state');
    }
    return [pA, pB];
  }

  getMetric(): number {
    switch (this.mCount) {
      case 0:
        return 0.0;
      case 1:
        return 0.0;
      case indexTwo:
        return b2Distance(this.mv1.w, this.mv2.w);
      case indexThr:
        return b2Cross(subtract(this.mv2.w, this.mv1.w), subtract(this.mv3.w, this.mv1.w));
      default:
        return 0.0;
    }
  }

  solve2(): void {
    let w1 = this.mv1.w;
    let w2 = this.mv2.w;
    let e12 = subtract(w2, w1);
    let d122 = -b2Dot22(w1, e12);
    if (d122 <= 0.0) {
      this.mv1.a = 1.0;
      this.mCount = 1;
      return;
    }
    let d121 = b2Dot22(w2, e12);
    if (d121 <= 0.0) {
      this.mv2.a = 1.0;
      this.mCount = 1;
      this.mv1 = this.mv2;
      return;
    }
    let invd12 = 1.0 / (d121 + d122);
    this.mv1.a = d121 * invd12;
    this.mv2.a = d122 * invd12;
    this.mCount = indexTwo;
  }
  
  solve3(): void {
    let w1 = this.mv1.w;
    let w2 = this.mv2.w;
    let w3 = this.mv3.w;
    let e12 = subtract(w2, w1);
    let w1e12 = b2Dot22(w1, e12);
    let w2e12 = b2Dot22(w2, e12);
    let d121 = w2e12;
    let d122 = -w1e12;
    let e13 = subtract(w3, w1);
    let w1e13 = b2Dot22(w1, e13);
    let w3e13 = b2Dot22(w3, e13);
    let d131 = w3e13;
    let d132 = -w1e13;
    let e23 = subtract(w3, w2);
    let w2e23 = b2Dot22(w2, e23);
    let w3e23 = b2Dot22(w3, e23);
    let d231 = w3e23;
    let d232 = -w2e23;
    let n123 = b2Cross(e12, e13);
    let d1231 = n123 * b2Cross(w2, w3);
    let d1232 = n123 * b2Cross(w3, w1);
    let d1233 = n123 * b2Cross(w1, w2);
    if (d122 <= 0.0 && d132 <= 0.0) {
      this.mv1.a = 1.0;
      this.mCount = 1;
      return;
    }
    if (d121 > 0.0 && d122 > 0.0 && d1233 <= 0.0) {
      let invd12 = 1.0 / (d121 + d122);
      this.mv1.a = d121 * invd12;
      this.mv2.a = d122 * invd12;
      this.mCount = indexTwo;
      return;
    }
    if (d131 > 0.0 && d132 > 0.0 && d1232 <= 0.0) {
      let invd13 = 1.0 / (d131 + d132);
      this.mv1.a = d131 * invd13;
      this.mv3.a = d132 * invd13;
      this.mCount = indexTwo;
      this.mv2 = this.mv3;
      return;
    }
    if (d121 <= 0.0 && d232 <= 0.0) {
      this.mv2.a = 1.0;
      this.mCount = 1;
      this.mv1 = this.mv2;
      return;
    }
    if (d131 <= 0.0 && d231 <= 0.0) {
      this.mv3.a = 1.0;
      this.mCount = 1;
      this.mv1 = this.mv3;
      return;
    }
    if (d231 > 0.0 && d232 > 0.0 && d1231 <= 0.0) {
      let invd23 = 1.0 / (d231 + d232);
      this.mv2.a = d231 * invd23;
      this.mv3.a = d232 * invd23;
      this.mCount = indexTwo;
      this.mv1 = this.mv3;
      return;
    }
    let invd123 = 1.0 / (d1231 + d1232 + d1233);
    this.mv1.a = d1231 * invd123;
    this.mv2.a = d1232 * invd123;
    this.mv3.a = d1233 * invd123;
    this.mCount = indexThr;
  }
  
  mv: B2SimplexVertex[] = [];
  get mv1(): B2SimplexVertex {
    return this.mv[0];
  }

  set mv1(newValue) {
    this.mv[0].set(newValue);
  }

  get mv2(): B2SimplexVertex {
    return this.mv[1];
  }

  set mv2(newValue) {
    this.mv[1].set(newValue);
  }

  get mv3(): B2SimplexVertex {
    return this.mv[float2];
  }

  set mv3(newValue) {
    this.mv[float2].set(newValue);
  }
}

export class B2SimplexCache {
  public metric: number;
  public count: number;
  public indexA: number[];
  public indexB: number[];
  public constructor() {
    this.metric = 0;
    this.count = 0;
    this.indexA = [0, 0, 0];
    this.indexB = [0, 0, 0];
  }
}

export class B2DistanceInput {
  public proxyA: B2DistanceProxy;
  public proxyB: B2DistanceProxy;
  public transformA: B2Transform;
  public transformB: B2Transform;
  public useRadii: boolean;
  public constructor() {
    this.proxyA = new B2DistanceProxy();
    this.proxyB = new B2DistanceProxy();
    this.transformA = new B2Transform();
    this.transformB = new B2Transform();
    this.useRadii = false;
  }
}

export class B2DistanceOutput {
  public pointA: B2Vec2;
  public pointB: B2Vec2;
  public distance: number;
  public iterations: number;
  public constructor() {
    this.pointA = new B2Vec2();
    this.pointB = new B2Vec2();
    this.distance = 0;
    this.iterations = 0;
  }
}

function b2DistanceM(output: B2DistanceOutput, cache: B2SimplexCache, input: B2DistanceInput): void {
  let proxyA = input.proxyA;
  let proxyB = input.proxyB;
  let transformA = input.transformA;
  let transformB = input.transformB;
  let simplex = new B2Simplex();
  simplex.readCache(cache, proxyA, transformA, proxyB, transformB);
  let kMaxIters = 20;
  let saveA = [0, 0, 0];
  let saveB = [0, 0, 0];
  let saveCount = 0;
  let distanceSqr1 = b2MaxFloat;
  let distanceSqr2 = distanceSqr1;
  let iter = 0;
  while (iter < kMaxIters) {
    saveCount = simplex.mCount;
    for (let i = 0; i < saveCount; i++) {
      saveA[i] = simplex.mv[i].indexA;
      saveB[i] = simplex.mv[i].indexB;
    }
    switch (simplex.mCount) {
      case 1:
        break;
      case indexTwo:
        simplex.solve2();
        break;
      case indexThr:
        simplex.solve3();
        break;
      default:
        break;
    }
    if (simplex.mCount === indexThr) {
      break;
    }
    let p = simplex.getClosestPoint();
    distanceSqr2 = p.lengthSquared();
    distanceSqr1 = distanceSqr2;
    let d = simplex.getSearchDirection();
    if (d.lengthSquared() < b2Epsilon * b2Epsilon) {
      break;
    }
    let vertex = simplex.mv[simplex.mCount];
    vertex.indexA = proxyA.getSupport(b2MulTR2(transformA.q, minus(d)));
    vertex.wA = b2MulT2(transformA, proxyA.getVertex(vertex.indexA));
    vertex.indexB = proxyB.getSupport(b2MulTR2(transformB.q, d));
    vertex.wB = b2MulT2(transformB, proxyB.getVertex(vertex.indexB));
    vertex.w = subtract(vertex.wB, vertex.wA);
    iter += 1;
    let duplicate = false;
    for (let i = 0; i < saveCount; i++) {
      if (vertex.indexA === saveA[i] && vertex.indexB === saveB[i]) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      break;
    }
    simplex.mCount += 1;
  }
  b2GjkMaxIters = Math.max(b2GjkMaxIters, iter);
  let sget = simplex.getWitnessPoints();
  output.pointA = sget[0];
  output.pointB = sget[1];
  output.distance = b2Distance(output.pointA, output.pointB);
  output.iterations = iter;
  simplex.writeCache(cache);
  if (input.useRadii) {
    let rA = proxyA.mRadius;
    let rB = proxyB.mRadius;
    if (output.distance > rA + rB && output.distance > b2Epsilon) {
      output.distance -= rA + rB;
      let normal = subtract(output.pointB, output.pointA);
      normal.normalize();
      addEqual(output.pointA, multM(normal, rA));
      subtractEqual(output.pointB, multM(normal, rB));
    } else {
      let p = multM(add(output.pointA, output.pointB), half);
      output.pointA = p;
      output.pointB = p;
      output.distance = 0.0;
    }
  }
  return;
}

let b2GjkMaxIters: number = 0;

export { b2DistanceM };
