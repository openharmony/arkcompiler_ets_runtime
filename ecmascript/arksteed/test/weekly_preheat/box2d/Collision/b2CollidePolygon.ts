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
import { B2Transform, B2Vec2 } from '../Common/b2Math';
import { b2MulT, b2MulR2, b2MulT2, b2MulTR2, b2Dot22, subtract, add, b2Cross12, multM, b2MulTT2, minus } from '../Common/b2Math';
import { B2PolygonShape } from './Shapes/b2Shape';
import { B2Manifold } from './b2Collision';
import { B2ClipVertex, B2ContactFeatureType, b2ClipSegmentToLine, B2ManifoldPoint, B2ManifoldType } from './b2Collision';
import { b2MaxFloat, b2LinearSlop, b2MaxManifoldPoints, float2, half, indexTwo } from '../Common/b2Settings';

function b2FindMaxSeparation(poly1: B2PolygonShape, xf1: B2Transform, poly2: B2PolygonShape, xf2: B2Transform): [edgeIndex: number, maxSeparation: number] {
  let count1 = poly1.mCount;
  let count2 = poly2.mCount;
  let n1s = poly1.mNormals;
  let v1s = poly1.mVertices;
  let v2s = poly2.mVertices;
  let xf = b2MulT(xf2, xf1);
  let bestIndex = 0;
  let maxSeparation = -b2MaxFloat;
  for (let i = 0; i < count1; i++) {
    let n = b2MulR2(xf.q, n1s.get(i));
    let v1 = b2MulT2(xf, v1s.get(i));
    let si = b2MaxFloat;
    for (let j = 0; j < count2; j++) {
      let sij = b2Dot22(n, subtract(v2s.get(j), v1));
      if (sij < si) {
        si = sij;
      }
    }
    if (si > maxSeparation) {
      maxSeparation = si;
      bestIndex = i;
    }
  }
  return [bestIndex, maxSeparation];
}

function b2FindIncidentEdge(poly1: B2PolygonShape, xf1: B2Transform, edge1: number, poly2: B2PolygonShape, xf2: B2Transform): B2ClipVertex[] {
  let normals1 = poly1.mNormals;
  let count2 = poly2.mCount;
  let vertices2 = poly2.mVertices;
  let normals2 = poly2.mNormals;
  let normal1 = b2MulTR2(xf2.q, b2MulR2(xf1.q, normals1.get(edge1)));
  let index = 0;
  let minDot = b2MaxFloat;
  for (let i = 0; i < count2; i++) {
    let dot = b2Dot22(normal1, normals2.get(i));
    if (dot < minDot) {
      minDot = dot;
      index = i;
    }
  }
  let i1 = index;
  let i2 = i1 + 1 < count2 ? i1 + 1 : 0;
  let c: B2ClipVertex[] = [];
  for (let i = 0; i < float2; i++) {
    c.push(new B2ClipVertex());
  }
  c[0].v = b2MulT2(xf2, vertices2.get(i1));
  c[0].id.indexA = edge1;
  c[0].id.indexB = i1;
  c[0].id.typeA = B2ContactFeatureType.FACE;
  c[0].id.typeB = B2ContactFeatureType.VERTEX;
  c[1].v = b2MulT2(xf2, vertices2.get(i2));
  c[1].id.indexA = edge1;
  c[1].id.indexB = i2;
  c[1].id.typeA = B2ContactFeatureType.FACE;
  c[1].id.typeB = B2ContactFeatureType.VERTEX;
  return c;
}

function b2CollidePolygons(manifold: B2Manifold, polyA: B2PolygonShape, xfA: B2Transform, polyB: B2PolygonShape, xfB: B2Transform): void {
  manifold.points.splice(0, manifold.points.length);
  let totalRadius = polyA.mRadius + polyB.mRadius;
  let fm = b2FindMaxSeparation(polyA, xfA, polyB, xfB);
  let edgeA = fm[0];
  let separationA = fm[1];
  if (separationA > totalRadius) {
    return;
  }
  let fmb = b2FindMaxSeparation(polyB, xfB, polyA, xfA);
  let edgeB = fmb[0];
  let separationB = fmb[1];
  if (separationB > totalRadius) {
    return;
  }
  let ratio: number = 0.1;
  let kTol: number = ratio * b2LinearSlop;
  let speAb = separationB > separationA + kTol;
  let poly1: B2PolygonShape = speAb ? polyB : polyA;
  let poly2: B2PolygonShape = speAb ? polyA : polyB;
  let xf1: B2Transform = speAb ? xfB : xfA;
  let xf2: B2Transform = speAb ? xfA : xfB;
  let edge1: number = speAb ? edgeB : edgeA;
  let flip: boolean = speAb ? true : false;
  manifold.type = speAb ? B2ManifoldType.FACEB : B2ManifoldType.FACEA;
  let incidentEdge = b2FindIncidentEdge(poly1, xf1, edge1, poly2, xf2);
  let vertices1 = poly1.mVertices;
  let iv2 = edge1 + 1 < poly1.mCount ? edge1 + 1 : 0;
  let v11: B2Vec2 = vertices1.get(edge1);
  let v12: B2Vec2 = vertices1.get(iv2);
  let localTangent = subtract(v12, v11);
  localTangent.normalize();
  let localNormal = b2Cross12(localTangent, 1.0);
  let planePoint = multM(add(v11, v12), half);
  let tangent = b2MulR2(xf1.q, localTangent);
  let normal = b2Cross12(tangent, 1.0);
  v11 = b2MulT2(xf1, v11);
  v12 = b2MulT2(xf1, v12);
  let frontOffset = b2Dot22(normal, v11);
  let sideOffset1 = -b2Dot22(tangent, v11) + totalRadius;
  let sideOffset2 = b2Dot22(tangent, v12) + totalRadius;
  let clipPoints1 = b2ClipSegmentToLine(incidentEdge, minus(tangent), sideOffset1, edge1);
  if (clipPoints1.length < indexTwo) {
    return;
  }
  let clipPoints2 = b2ClipSegmentToLine(clipPoints1, tangent, sideOffset2, iv2);
  if (clipPoints2.length < indexTwo) {
    return;
  }
  manifold.localNormal = localNormal;
  manifold.localPoint = planePoint;
  for (let i = 0; i < b2MaxManifoldPoints; i++) {
    let separation = b2Dot22(normal, clipPoints2[i].v) - frontOffset;
    if (separation <= totalRadius) {
      let cp = new B2ManifoldPoint();
      cp.localPoint = b2MulTT2(xf2, clipPoints2[i].v);
      cp.id = clipPoints2[i].id;
      if (flip) {
        let cf = cp.id;
        cp.id.indexA = cf.indexB;
        cp.id.indexB = cf.indexA;
        cp.id.typeA = cf.typeB;
        cp.id.typeB = cf.typeA;
      }
      manifold.points.push(cp);
    }
  }
  return;
}
export { b2CollidePolygons };
