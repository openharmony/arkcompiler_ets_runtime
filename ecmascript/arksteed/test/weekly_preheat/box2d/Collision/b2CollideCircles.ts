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
import { B2Vec2, B2Transform } from '../Common/b2Math';
import { b2DistanceSquared, b2MulT2, b2Dot22, add, subtract, multM, b2MulTT2 } from '../Common/b2Math';
import { B2ManifoldType, B2ManifoldPoint } from './b2Collision';
import { B2Manifold } from './b2Collision';
import { B2CircleShape, B2PolygonShape } from './Shapes/b2Shape';
import { b2Epsilon, b2MaxFloat, half } from '../Common/b2Settings';

function b2CollideCircles(manifold: B2Manifold, circleA: B2CircleShape, xfA: B2Transform, circleB: B2CircleShape, xfB: B2Transform): void {
  manifold.points.splice(0, manifold.points.length);
  let pA = b2MulT2(xfA, circleA.mPoint);
  let pB = b2MulT2(xfB, circleB.mPoint);
  let d = subtract(pB, pA);
  let distSqr = b2Dot22(d, d);
  let rA = circleA.mRadius;
  let rB = circleB.mRadius;
  let radius = rA + rB;
  if (distSqr > radius * radius) {
    return;
  }
  manifold.type = B2ManifoldType.CIRCLES;
  manifold.localPoint = circleA.mPoint;
  manifold.localNormal.setZero();
  let cp = new B2ManifoldPoint();
  cp.localPoint = circleB.mPoint;
  cp.id.setZero();
  manifold.points.push(cp);
}

function b2CollidePolygonAndCircle(manifold: B2Manifold, polygonA: B2PolygonShape, xfA: B2Transform, circleB: B2CircleShape, xfB: B2Transform): void {
  manifold.points.splice(0, manifold.points.length);
  let c = b2MulT2(xfB, circleB.mPoint);
  let cLocal = b2MulTT2(xfA, c);
  let normalIndex = 0;
  let separation = -b2MaxFloat;
  let radius = polygonA.mRadius + circleB.mRadius;
  let vertexCount = polygonA.mCount;
  let vertices = polygonA.mVertices;
  let normals = polygonA.mNormals;
  for (let i = 0; i < vertexCount; i++) {
    let n = subtract(cLocal, vertices.get(i));
    let s = b2Dot22(normals.get(i), n);
    if (s > radius) {
      return;
    }
    if (s > separation) {
      separation = s;
      normalIndex = i;
    }
  }
  let vertIndex1 = normalIndex;
  let vertIndex2 = vertIndex1 + 1 < vertexCount ? vertIndex1 + 1 : 0;
  let v1: B2Vec2 = vertices.get(vertIndex1);
  let v2: B2Vec2 = vertices.get(vertIndex2);
  if (separation < b2Epsilon) {
    manifold.type = B2ManifoldType.FACEA;
    manifold.localNormal = normals.get(normalIndex);
    manifold.localPoint = multM(add(v1, v2), half);
    let cp = new B2ManifoldPoint();
    cp.localPoint = circleB.mPoint;
    cp.id.setZero();
    manifold.points.push(cp);
    return;
  }
  let u1 = b2Dot22(subtract(cLocal, v1), subtract(v2, v1));
  let u2 = b2Dot22(subtract(cLocal, v2), subtract(v1, v2));
  if (u1 <= 0.0) {
    if (b2DistanceSquared(cLocal, v1) > radius * radius) {
      return;
    }
    manifold.type = B2ManifoldType.FACEA;
    manifold.localNormal = subtract(cLocal, v1);
    manifold.localNormal.normalize();
    manifold.localPoint = v1;
    let cp = new B2ManifoldPoint();
    cp.localPoint = circleB.mPoint;
    cp.id.setZero();
    manifold.points.push(cp);
  } else if (u2 <= 0.0) {
    if (b2DistanceSquared(cLocal, v2) > radius * radius) {
      return;
    }
    manifold.type = B2ManifoldType.FACEA;
    manifold.localNormal = subtract(cLocal, v2);
    manifold.localNormal.normalize();
    manifold.localPoint = v2;
    let cp = new B2ManifoldPoint();
    cp.localPoint = circleB.mPoint;
    cp.id.setZero();
    manifold.points.push(cp);
  } else {
    let inVec = normals.get(vertIndex1);
    let faceCenter = multM(add(v1, v2), half);
    let separation = b2Dot22(subtract(cLocal, faceCenter), inVec);
    if (separation > radius) {
      return;
    }
    manifold.type = B2ManifoldType.FACEA;
    manifold.localNormal = inVec;
    manifold.localPoint = faceCenter;
    let cp = new B2ManifoldPoint();
    cp.localPoint = circleB.mPoint;
    cp.id.setZero();
    manifold.points.push(cp);
  }
  return;
}

export { b2CollideCircles, b2CollidePolygonAndCircle };
