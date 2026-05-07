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
import { B2Transform } from '../../Common/b2Math';
import { B2Vec2, add, b2MulR2, subtract, b2Dot22, b2Sqrt, multM, b2MulT2, b2Min, b2Max, addEqual, mulEqual, b2Cross } from '../../Common/b2Math';
import { half, indexTwo } from '../../Common/b2Settings';
import { float025 } from '../../Common/b2Settings';
import { float3 } from '../../Common/b2Settings';
import { b2Epsilon, b2PI, b2PolygonRadius } from '../../Common/b2Settings';
import { B2Array } from '../../Dynamics/b2TimeStep';
import { B2RayCastInput, B2RayCastOutput, B2AaBb } from '../b2Collision';

export class B2MassData {
  public mass: number = 0;
  public center = new B2Vec2();
  public aI: number = 0;
}

enum B2ShapeType {
  CIRCLE,
  EDGE,
  POLYGON,
  CHAIN,
  TYPECOUNT
}

export class B2Shape {
  constructor() {
    this.mType = B2ShapeType.CIRCLE;
    this.mRadius = 0;
  }

  public clone(): B2Shape {
    throw new Error('must override');
  }

  public type(): B2ShapeType {
    return this.mType;
  }

  public childCount(): number {
    throw new Error('must override');
  }

  public testPoint(transform: B2Transform, point: B2Vec2): boolean {
    throw new Error('must override');
  }

  public rayCast(output: B2RayCastOutput, input: B2RayCastInput, transform: B2Transform, childIndex: number): boolean {
    throw new Error('must override');
  }

  public computeAaBb(aabb: B2AaBb, transform: B2Transform, childIndex: number): void {
    throw new Error('must override');
  }

  public computeMass(density: number): B2MassData {
    throw new Error('must override');
  }

  public mType: B2ShapeType;
  public mRadius: number;
  get radius(): number {
    return this.mRadius;
  }
  set radius(newValue) {
    this.mRadius = newValue;
  }
}

export class B2CircleShape extends B2Shape {
  constructor() {
    super();
    this.mType = B2ShapeType.CIRCLE;
    this.mRadius = 0.0;
    this.mPoint = new B2Vec2(0.0, 0.0);
  }

  public clone(): B2Shape {
    let clone = new B2CircleShape();
    clone.mRadius = this.mRadius;
    clone.mPoint = this.mPoint;
    return clone;
  }

  public childCount(): number {
    return 1;
  }

  public testPoint(transform: B2Transform, point: B2Vec2): boolean {
    let center = add(transform.p, b2MulR2(transform.q, this.mPoint));
    let d = subtract(this.p, center);
    return b2Dot22(d, d) <= this.mRadius * this.mRadius;
  }

  public rayCast(output: B2RayCastOutput, input: B2RayCastInput, transform: B2Transform, childIndex: number): boolean {
    let position = add(transform.p, b2MulR2(transform.q, this.mPoint));
    let s = subtract(input.p1, position);
    let b = b2Dot22(s, s) - this.mRadius * this.mRadius;
    let r = subtract(input.p2, input.p1);
    let c = b2Dot22(s, r);
    let rr = b2Dot22(r, r);
    let sigma = c * c - rr * b;
    if (sigma < 0.0 || rr < b2Epsilon) {
      return false;
    }
    let a = -(c + b2Sqrt(sigma));
    if (0.0 <= a && a <= input.maxFraction * rr) {
      a /= rr;
      output.fraction = a;
      output.normal = add(s, multM(r, a));
      output.normal.normalize();
      return true;
    }
    return false;
  }

  public computeAaBb(aabb: B2AaBb, transform: B2Transform, childIndex: number): void {
    let p = add(transform.p, b2MulR2(transform.q, this.mPoint));
    aabb.lowerBound.set(p.x - this.mRadius, p.y - this.mRadius);
    aabb.upperBound.set(p.x + this.mRadius, p.y + this.mRadius);
  }

  public computeMass(density: number): B2MassData {
    let massData = new B2MassData();
    massData.mass = density * b2PI * this.mRadius * this.mRadius;
    massData.center = this.mPoint;
    massData.aI = massData.mass * (half * this.mRadius * this.mRadius + b2Dot22(this.mPoint, this.mPoint));
    return massData;
  }

  public getSupport(direction: B2Vec2): number {
    return 0;
  }

  public getSupportVertex(direction: B2Vec2): B2Vec2 {
    return this.mPoint;
  }

  public vertexCount(): number {
    return 1;
  }

  public vertex(index: number): B2Vec2 {
    return this.mPoint;
  }

  get p(): B2Vec2 {
    return this.mp.get(0);
  }
  set p(newValue: B2Vec2) {
    this.mp.set(0, newValue);
  }
  mp = new B2Array<B2Vec2>(1, new B2Vec2());
  get mPoint(): B2Vec2 {
    return this.mp.get(0);
  }
  set mPoint(newValue: B2Vec2) {
    this.mp.set(0, newValue);
  }
}

export class B2EdgeShape extends B2Shape {
  constructor() {
    super();
    this.mVertex0 = new B2Vec2(0.0, 0.0);
    this.mVertex3 = new B2Vec2(0.0, 0.0);
    this.mHasVertex0 = false;
    this.mHasVertex3 = false;
    this.mType = B2ShapeType.EDGE;
    this.mRadius = b2PolygonRadius;
  }

  public set(v1: B2Vec2, v2: B2Vec2): void {
    this.mVertex1 = v1;
    this.mVertex2 = v2;
    this.mHasVertex0 = false;
    this.mHasVertex3 = false;
  }

  public clone(): B2Shape {
    let clone = new B2EdgeShape();
    clone.mRadius = this.mRadius;
    clone.mVertices = this.mVertices.clone();
    clone.mVertex0 = this.mVertex0;
    clone.mVertex3 = this.mVertex3;
    clone.mHasVertex0 = this.mHasVertex0;
    clone.mHasVertex3 = this.mHasVertex3;
    return clone;
  }

  public childCount(): number {
    return 1;
  }

  public testPoint(transform: B2Transform, point: B2Vec2): boolean {
    return false;
  }

  public computeAaBb(aabb: B2AaBb, transform: B2Transform, childIndex: number): void {
    let v1 = b2MulT2(transform, this.mVertex1);
    let v2 = b2MulT2(transform, this.mVertex2);
    let lower = b2Min(v1, v2);
    let upper = b2Max(v1, v2);
    let r = new B2Vec2(this.mRadius, this.mRadius);
    aabb.lowerBound = subtract(lower, r);
    aabb.upperBound = add(upper, r);
  }

  public computeMass(density: number): B2MassData {
    let massData = new B2MassData();
    massData.mass = 0.0;
    massData.center = multM(add(this.mVertex1, this.mVertex2), half);
    massData.aI = 0.0;
    return massData;
  }

  get vertex1(): B2Vec2 {
    return this.mVertices.get(0);
  }
  set vertex1(newValue: B2Vec2) {
    this.mVertices.set(0, newValue);
  }

  get vertex2(): B2Vec2 {
    return this.mVertices.get(1);
  }
  set vertex2(newValue: B2Vec2) {
    this.mVertices.set(1, newValue);
  }

  get vertex0(): B2Vec2 {
    return this.mVertex0;
  }
  set vertex0(newValue) {
    this.mVertex0 = newValue;
  }

  get vertex3(): B2Vec2 {
    return this.mVertex3;
  }
  set vertex3(newValue) {
    this.mVertex3 = newValue;
  }

  get hasVertex0(): boolean {
    return this.mHasVertex0;
  }
  set hasVertex0(newValue) {
    this.mHasVertex0 = newValue;
  }

  get hasVertex3(): boolean {
    return this.mHasVertex3;
  }
  set hasVertex3(newValue) {
    this.mHasVertex3 = newValue;
  }

  public mVertices = new B2Array<B2Vec2>(indexTwo, new B2Vec2());
  get mVertex1(): B2Vec2 {
    return this.mVertices.get(0);
  }
  set mVertex1(newValue: B2Vec2) {
    this.mVertices.set(0, newValue);
  }
  get mVertex2(): B2Vec2 {
    return this.mVertices.get(1);
  }
  set mVertex2(newValue: B2Vec2) {
    this.mVertices.set(1, newValue);
  }

  public mVertex0: B2Vec2;
  public mVertex3: B2Vec2;
  public mHasVertex0: boolean;
  public mHasVertex3: boolean;
}

export class B2PolygonShape extends B2Shape {
  constructor() {
    super();
    this.mCentroid = new B2Vec2(0.0, 0.0);
    this.mType = B2ShapeType.POLYGON;
    this.mRadius = b2PolygonRadius;
  }

  public clone(): B2Shape {
    let clone = new B2PolygonShape();
    clone.mCentroid = this.mCentroid;
    clone.mVertices = this.mVertices.clone();
    clone.mNormals = this.mNormals.clone();
    clone.mRadius = this.mRadius;
    return clone;
  }

  public childCount(): number {
    return 1;
  }

  public setAsEdge(v1: B2Vec2, v2: B2Vec2): void {
    this.mVertices.append(v1);
    this.mVertices.append(v2);
    this.mCentroid.x = half * (v1.x + v2.x);
    this.mCentroid.y = half * (v1.y + v2.y);
    this.mNormals.append(this.crossVF(this.subtractVV(v2, v1), 1.0));
    let mn0 = this.mNormals.get(0);
    mn0.normalize();
    this.mNormals.append(new B2Vec2(-this.mNormals.get(0).x, -this.mNormals.get(0).y));
  }

  public setAsBox(hx: number, hy: number): void {
    this.mVertices.removeAll();
    this.mVertices.append(new B2Vec2(-hx, -hy));
    this.mVertices.append(new B2Vec2(hx, -hy));
    this.mVertices.append(new B2Vec2(hx, hy));
    this.mVertices.append(new B2Vec2(-hx, hy));
    this.mNormals.removeAll();
    this.mNormals.append(new B2Vec2(0.0, -1.0));
    this.mNormals.append(new B2Vec2(1.0, 0.0));
    this.mNormals.append(new B2Vec2(0.0, 1.0));
    this.mNormals.append(new B2Vec2(-1.0, 0.0));
    this.mCentroid.setZero();
  }

  public subtractVV(a: B2Vec2, b: B2Vec2): B2Vec2 {
    let v = new B2Vec2(a.x - b.x, a.y - b.y);
    return v;
  }

  public crossVV(a: B2Vec2, b: B2Vec2): number {
    return a.x * b.y - a.y * b.x;
  }

  public crossVF(a: B2Vec2, s: number): B2Vec2 {
    let v = new B2Vec2(s * a.y, -s * a.x);
    return v;
  }

  public computeAaBb(aabb: B2AaBb, transform: B2Transform, childIndex: number): void {
    let lower = b2MulT2(transform, this.mVertices.get(0));
    let upper = lower;
    for (let i = 1; i < this.mCount; i++) {
      let v = b2MulT2(transform, this.mVertices.get(i));
      lower = b2Min(lower, v);
      upper = b2Max(upper, v);
    }
    let r = new B2Vec2(this.mRadius, this.mRadius);
    aabb.lowerBound = subtract(lower, r);
    aabb.upperBound = add(r, upper);
  }

  public computeMass(density: number): B2MassData {
    let center = new B2Vec2(0.0, 0.0);
    let area: number = 0.0;
    let mI: number = 0.0;
    let s = new B2Vec2(0.0, 0.0);
    for (let i = 0; i < this.mCount; i++) {
      addEqual(s, this.mVertices.get(i));
    }
    mulEqual(s, 1.0 / this.mVertices.count);
    let inv3: number = 1.0 / float3;
    for (let i = 0; i < this.mCount; i++) {
      let e1: B2Vec2 = subtract(this.mVertices.get(i), s);
      let e2: B2Vec2 = i + 1 < this.mCount ? subtract(this.mVertices.get(i + 1), s) : subtract(this.mVertices.get(0), s);
      let aD = b2Cross(e1, e2);
      let triangleArea = half * aD;
      area += triangleArea;
      addEqual(center, multM(add(e1, e2), inv3 * triangleArea));
      let ex1 = e1.x;
      let ey1 = e1.y;
      let ex2 = e2.x;
      let ey2 = e2.y;
      let intx2 = ex1 * ex1 + ex2 * ex1 + ex2 * ex2;
      let inty2 = ey1 * ey1 + ey2 * ey1 + ey2 * ey2;
      mI += float025 * inv3 * aD * (intx2 + inty2);
    }
    let massData = new B2MassData();
    massData.mass = density * area;
    mulEqual(center, 1.0 / area);
    massData.center = add(center, s);
    massData.aI = density * mI;
    massData.aI += massData.mass * (b2Dot22(massData.center, massData.center) - b2Dot22(center, center));
    return massData;
  }

  public vertexCount(): number {
    return this.mCount;
  }

  public vertex(index: number): B2Vec2 {
    return this.mVertices.get(index);
  }
  public validate(): boolean {
    for (let i = 0; i < this.mVertices.count; i++) {
      let i1 = i;
      let i2 = i < this.mVertices.count - 1 ? i1 + 1 : 0;
      let p: B2Vec2 = this.mVertices.get(i1);
      let e: B2Vec2 = subtract(this.mVertices.get(i2), p);
      for (let j = 0; j < this.mCount; j++) {
        if (j === i1 || j === i2) {
          continue;
        }
        let v: B2Vec2 = subtract(this.mVertices.get(j), p);
        let c = b2Cross(e, v);
        if (c < 0.0) {
          return false;
        }
      }
    }
    return true;
  }

  get vertices(): B2Array<B2Vec2> {
    return this.mVertices;
  }

  get count(): number {
    return this.mCount;
  }

  mCentroid = new B2Vec2();
  mVertices = new B2Array<B2Vec2>();
  mNormals = new B2Array<B2Vec2>();
  get mCount(): number {
    return this.mVertices.count;
  }
}

export { B2ShapeType };
