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
import { B2Transform, B2Vec2, b2MulT, b2MulT2, subtract, b2Dot22, multM, b2Cross, b2MulR2, b2MulTT2, add, minus } from '../Common/b2Math';
import { B2Manifold, B2ContactFeature, B2ContactFeatureType, B2ManifoldType, B2ManifoldPoint, B2ClipVertex, b2ClipSegmentToLine } from './b2Collision';
import { B2EdgeShape, B2CircleShape, B2PolygonShape } from './Shapes/b2Shape';
import { b2MaxPolygonVertices, b2PolygonRadius, b2MaxManifoldPoints, b2AngularSlop, b2MaxFloat, float2, indexTwo } from '../Common/b2Settings';
function b2CollideEdgeAndCircle(manifold: B2Manifold, edgeA: B2EdgeShape, xfA: B2Transform, circleB: B2CircleShape, xfB: B2Transform): void {
  manifold.points.splice(0, manifold.points.length);
  let mQ = b2MulTT2(xfA, b2MulT2(xfB, circleB.mPoint));
  let eA: B2Vec2 = edgeA.mVertex1;
  let eB: B2Vec2 = edgeA.mVertex2;
  let e = subtract(eB, eA);
  let u = b2Dot22(e, subtract(eB, mQ));
  let v = b2Dot22(e, subtract(mQ, eA));
  let radius = edgeA.mRadius + circleB.mRadius;
  let cf = new B2ContactFeature();
  cf.indexB = 0;
  cf.typeB = B2ContactFeatureType.VERTEX;
  if (v <= 0.0) {
    let mP = eA;
    let d = subtract(mQ, mP);
    let dd = b2Dot22(d, d);
    if (dd > radius * radius) {
      return;
    }
    if (edgeA.mHasVertex0) {
      let aA1 = edgeA.mVertex0;
      let aB1 = eA;
      let e1 = subtract(aB1, aA1);
      let u1 = b2Dot22(e1, subtract(aB1, mQ));

      if (u1 > 0.0) {
        return;
      }
    }
    cf.indexA = 0;
    cf.typeA = B2ContactFeatureType.VERTEX;
    manifold.type = B2ManifoldType.CIRCLES;
    manifold.localNormal.setZero();
    manifold.localPoint = mP;
    let cp = new B2ManifoldPoint();
    cp.id.setZero();
    cp.id = cf;
    cp.localPoint = circleB.mPoint;
    manifold.points.push(cp);
    return;
  }
  if (u <= 0.0) {
    let mP = eB;
    let d = subtract(mQ, mP);
    let dd = b2Dot22(d, d);
    if (dd > radius * radius) {
      return;
    }
    if (edgeA.mHasVertex3) {
      let vB2 = edgeA.mVertex3;
      let vA2 = eB;
      let e2 = subtract(vB2, vA2);
      let v2 = b2Dot22(e2, subtract(mQ, vA2));
      if (v2 > 0.0) {
        return;
      }
    }
    cf.indexA = 1;
    cf.typeA = B2ContactFeatureType.VERTEX;
    manifold.type = B2ManifoldType.CIRCLES;
    manifold.localNormal.setZero();
    manifold.localPoint = mP;
    let cp = new B2ManifoldPoint();
    cp.id.setZero();
    cp.id = cf;
    cp.localPoint = circleB.mPoint;
    manifold.points.push(cp);
    return;
  }
  let den = b2Dot22(e, e);
  let vP = multM(add(multM(eA, u), multM(eB, v)), 1.0 / den);
  let d = subtract(mQ, vP);
  let dd = b2Dot22(d, d);
  if (dd > radius * radius) {
    return;
  }
  let n = new B2Vec2(-e.y, e.x);
  if (b2Dot22(n, subtract(mQ, eA)) < 0.0) {
    n.set(-n.x, -n.y);
  }
  n.normalize();
  cf.indexA = 0;
  cf.typeA = B2ContactFeatureType.FACE;
  manifold.type = B2ManifoldType.FACEA;
  manifold.localNormal = n;
  manifold.localPoint = eA;
  let cp = new B2ManifoldPoint();
  cp.id.setZero();
  cp.id = cf;
  cp.localPoint = circleB.mPoint;
  manifold.points.push(cp);
  return;
}

enum B2EpAxisType {
  UNKNOWN,
  EDGEA,
  EDGEB
}

export class B2EpAxis {
  type = B2EpAxisType.UNKNOWN;
  index = 0;
  separation: number = 0;
}

export class B2TempPolygon {
  vertices: B2Vec2[] = [];
  normals: B2Vec2[] = [];
  count = 0;

  constructor() {
    for (let i = 0; i < b2MaxPolygonVertices; i++) {
      this.vertices.push(new B2Vec2());
      this.normals.push(new B2Vec2());
    }
  }
}

export class B2ReferenceFace {
  i1 = 0;
  i2 = 0;
  v1 = new B2Vec2();
  v2 = new B2Vec2();
  normal = new B2Vec2();
  sideNormal1 = new B2Vec2();
  sideOffset1: number = 0;
  sideNormal2 = new B2Vec2();
  sideOffset2: number = 0;
}
export class B2EpCollider {
  mPolygonB = new B2TempPolygon();
  mxf = new B2Transform();
  mCentroidB = new B2Vec2();
  mv0 = new B2Vec2();
  mv1 = new B2Vec2();
  mv2 = new B2Vec2();
  mv3 = new B2Vec2();
  mNormal0 = new B2Vec2();
  mNormal1 = new B2Vec2();
  mNormal2 = new B2Vec2();
  mNormal = new B2Vec2();
  mType1 = VertexType.ISOLATED;
  mType2 = VertexType.ISOLATED;
  mLowerLimit = new B2Vec2();
  mUpperLimit = new B2Vec2();
  mRadius: number = 0;
  mFront = false;

  public collide(edgeA: B2EdgeShape, xfA: B2Transform, polygonB: B2PolygonShape, xfB: B2Transform): B2Manifold {
    let manifold = new B2Manifold();
    this.mxf = b2MulT(xfA, xfB);
    this.mCentroidB = b2MulT2(this.mxf, polygonB.mCentroid);
    this.mv0 = edgeA.mVertex0;
    this.mv1 = edgeA.mVertex1;
    this.mv2 = edgeA.mVertex2;
    this.mv3 = edgeA.mVertex3;
    let hasVertex0 = edgeA.mHasVertex0;
    let hasVertex3 = edgeA.mHasVertex3;
    let edge1 = subtract(this.mv2, this.mv1);
    edge1.normalize();
    this.mNormal1.set(edge1.y, -edge1.x);
    let offset1 = b2Dot22(this.mNormal1, subtract(this.mCentroidB, this.mv1));
    let offset0: number = 0.0;
    let offset2: number = 0.0;
    let convex1 = false;
    let convex2 = false;
    if (hasVertex0) {
      let edge0 = subtract(this.mv1, this.mv0);
      edge0.normalize();
      this.mNormal0.set(edge0.y, -edge0.x);
      convex1 = b2Cross(edge0, edge1) >= 0.0;
      offset0 = b2Dot22(this.mNormal0, subtract(this.mCentroidB, this.mv0));
    }
    if (hasVertex3) {
      let edge2 = subtract(this.mv3, this.mv2);
      edge2.normalize();
      this.mNormal2.set(edge2.y, -edge2.x);
      convex2 = b2Cross(edge1, edge2) > 0.0;
      offset2 = b2Dot22(this.mNormal2, subtract(this.mCentroidB, this.mv2));
    }
    if (hasVertex0 && hasVertex3) {
      if (convex1 && convex2) {
        this.mFront = offset0 >= 0.0 || offset1 >= 0.0 || offset2 >= 0.0;
        if (this.mFront) {
          this.mNormal = this.mNormal1;
          this.mLowerLimit = this.mNormal0;
          this.mUpperLimit = this.mNormal2;
        } else {
          this.mNormal = minus(this.mNormal1);
          this.mLowerLimit = minus(this.mNormal1);
          this.mUpperLimit = minus(this.mNormal1);
        }
      } else if (convex1) {
        this.mFront = offset0 >= 0.0 || (offset1 >= 0.0 && offset2 >= 0.0);
        if (this.mFront) {
          this.mNormal = this.mNormal1;
          this.mLowerLimit = this.mNormal0;
          this.mUpperLimit = this.mNormal1;
        } else {
          this.mNormal = minus(this.mNormal1);
          this.mLowerLimit = minus(this.mNormal2);
          this.mUpperLimit = minus(this.mNormal1);
        }
      } else if (convex2) {
        this.mFront = offset2 >= 0.0 || (offset0 >= 0.0 && offset1 >= 0.0);
        if (this.mFront) {
          this.mNormal = this.mNormal1;
          this.mLowerLimit = this.mNormal1;
          this.mUpperLimit = this.mNormal2;
        } else {
          this.mNormal = minus(this.mNormal1);
          this.mLowerLimit = minus(this.mNormal1);
          this.mUpperLimit = minus(this.mNormal0);
        }
      } else {
        this.mFront = offset0 >= 0.0 && offset1 >= 0.0 && offset2 >= 0.0;
        if (this.mFront) {
          this.mNormal = this.mNormal1;
          this.mLowerLimit = this.mNormal1;
          this.mUpperLimit = this.mNormal1;
        } else {
          this.mNormal = minus(this.mNormal1);
          this.mLowerLimit = minus(this.mNormal2);
          this.mUpperLimit = minus(this.mNormal0);
        }
      }
    } else if (hasVertex0) {
      if (convex1) {
        this.mFront = offset0 >= 0.0 || offset1 >= 0.0;
        if (this.mFront) {
          this.mNormal = this.mNormal1;
          this.mLowerLimit = this.mNormal0;
          this.mUpperLimit = minus(this.mNormal1);
        } else {
          this.mNormal = minus(this.mNormal1);
          this.mLowerLimit = this.mNormal1;
          this.mUpperLimit = minus(this.mNormal1);
        }
      } else {
        this.mFront = offset0 >= 0.0 && offset1 >= 0.0;
        if (this.mFront) {
          this.mNormal = this.mNormal1;
          this.mLowerLimit = this.mNormal1;
          this.mUpperLimit = minus(this.mNormal1);
        } else {
          this.mNormal = minus(this.mNormal1);
          this.mLowerLimit = this.mNormal1;
          this.mUpperLimit = minus(this.mNormal0);
        }
      }
    } else if (hasVertex3) {
      if (convex2) {
        this.mFront = offset1 >= 0.0 || offset2 >= 0.0;
        if (this.mFront) {
          this.mNormal = this.mNormal1;
          this.mLowerLimit = minus(this.mNormal1);
          this.mUpperLimit = this.mNormal2;
        } else {
          this.mNormal = minus(this.mNormal1);
          this.mLowerLimit = minus(this.mNormal1);
          this.mUpperLimit = this.mNormal1;
        }
      } else {
        this.mFront = offset1 >= 0.0 && offset2 >= 0.0;
        if (this.mFront) {
          this.mNormal = this.mNormal1;
          this.mLowerLimit = minus(this.mNormal1);
          this.mUpperLimit = this.mNormal1;
        } else {
          this.mNormal = minus(this.mNormal1);
          this.mLowerLimit = minus(this.mNormal2);
          this.mUpperLimit = this.mNormal1;
        }
      }
    } else {
      this.mFront = offset1 >= 0.0;
      if (this.mFront) {
        this.mNormal = this.mNormal1;
        this.mLowerLimit = minus(this.mNormal1);
        this.mUpperLimit = minus(this.mNormal1);
      } else {
        this.mNormal = minus(this.mNormal1);
        this.mLowerLimit = this.mNormal1;
        this.mUpperLimit = this.mNormal1;
      }
    }
    this.mPolygonB.count = polygonB.mCount;
    for (let i = 0; i < polygonB.mCount; i++) {
      this.mPolygonB.vertices[i] = b2MulT2(this.mxf, polygonB.mVertices.get(i));
      this.mPolygonB.normals[i] = b2MulR2(this.mxf.q, polygonB.mNormals.get(i));
    }
    this.mRadius = float2 * b2PolygonRadius;
    manifold.points.splice(0, manifold.points.length);
    let edgeAxis = this.computeEdgeSeparation();
    if (edgeAxis.type === B2EpAxisType.UNKNOWN) {
      return manifold;
    }
    if (edgeAxis.separation > this.mRadius) {
      return manifold;
    }
    let polygonAxis = this.computePolygonSeparation();
    if (polygonAxis.type !== B2EpAxisType.UNKNOWN && polygonAxis.separation > this.mRadius) {
      return manifold;
    }
    let relativeTol: number = 0.98;
    let absoluteTol: number = 0.001;
    let primaryAxis: B2EpAxis;
    if (polygonAxis.type === B2EpAxisType.UNKNOWN) {
      primaryAxis = edgeAxis;
    } else if (polygonAxis.separation > relativeTol * edgeAxis.separation + absoluteTol) {
      primaryAxis = polygonAxis;
    } else {
      primaryAxis = edgeAxis;
    }
    let ie: B2ClipVertex[] = [];
    for (let i = 0; i < indexTwo; i++) {
      ie.push(new B2ClipVertex());
    }
    let rf = new B2ReferenceFace();
    if (primaryAxis.type === B2EpAxisType.EDGEA) {
      manifold.type = B2ManifoldType.FACEA;
      let bestIndex = 0;
      let bestValue = b2Dot22(this.mNormal, this.mPolygonB.normals[0]);
      for (let i = 1; i < this.mPolygonB.count; i++) {
        let value = b2Dot22(this.mNormal, this.mPolygonB.normals[i]);
        if (value < bestValue) {
          bestValue = value;
          bestIndex = i;
        }
      }
      let i1 = bestIndex;
      let i2 = i1 + 1 < this.mPolygonB.count ? i1 + 1 : 0;
      ie[0].v = this.mPolygonB.vertices[i1];
      ie[0].id.indexA = 0;
      ie[0].id.indexB = i1;
      ie[0].id.typeA = B2ContactFeatureType.FACE;
      ie[0].id.typeB = B2ContactFeatureType.VERTEX;
      ie[1].v = this.mPolygonB.vertices[i2];
      ie[1].id.indexA = 0;
      ie[1].id.indexB = i2;
      ie[1].id.typeA = B2ContactFeatureType.FACE;
      ie[1].id.typeB = B2ContactFeatureType.VERTEX;
      if (this.mFront) {
        rf.i1 = 0;
        rf.i2 = 1;
        rf.v1 = this.mv1;
        rf.v2 = this.mv2;
        rf.normal = this.mNormal1;
      } else {
        rf.i1 = 1;
        rf.i2 = 0;
        rf.v1 = this.mv2;
        rf.v2 = this.mv1;
        rf.normal = minus(this.mNormal1);
      }
    } else {
      manifold.type = B2ManifoldType.FACEB;
      ie[0].v = this.mv1;
      ie[0].id.indexA = 0;
      ie[0].id.indexB = primaryAxis.index;
      ie[0].id.typeA = B2ContactFeatureType.VERTEX;
      ie[0].id.typeB = B2ContactFeatureType.FACE;
      ie[1].v = this.mv2;
      ie[1].id.indexA = 0;
      ie[1].id.indexB = primaryAxis.index;
      ie[1].id.typeA = B2ContactFeatureType.VERTEX;
      ie[1].id.typeB = B2ContactFeatureType.FACE;
      rf.i1 = primaryAxis.index;
      rf.i2 = rf.i1 + 1 < this.mPolygonB.count ? rf.i1 + 1 : 0;
      rf.v1 = this.mPolygonB.vertices[rf.i1];
      rf.v2 = this.mPolygonB.vertices[rf.i2];
      rf.normal = this.mPolygonB.normals[rf.i1];
    }
    rf.sideNormal1.set(rf.normal.y, -rf.normal.x);
    rf.sideNormal2 = minus(rf.sideNormal1);
    rf.sideOffset1 = b2Dot22(rf.sideNormal1, rf.v1);
    rf.sideOffset2 = b2Dot22(rf.sideNormal2, rf.v2);
    let clipPoints1 = b2ClipSegmentToLine(ie, rf.sideNormal1, rf.sideOffset1, rf.i1);
    if (clipPoints1.length < b2MaxManifoldPoints) {
      return manifold;
    }
    let clipPoints2 = b2ClipSegmentToLine(clipPoints1, rf.sideNormal2, rf.sideOffset2, rf.i2);
    if (clipPoints2.length < b2MaxManifoldPoints) {
      return manifold;
    }
    if (primaryAxis.type === B2EpAxisType.EDGEA) {
      manifold.localNormal = rf.normal;
      manifold.localPoint = rf.v1;
    } else {
      manifold.localNormal = polygonB.mNormals.get(rf.i1);
      manifold.localPoint = polygonB.mVertices.get(rf.i1);
    }
    for (let i = 0; i < b2MaxManifoldPoints; i++) {
      let separation = b2Dot22(rf.normal, subtract(clipPoints2[i].v, rf.v1));
      if (separation <= this.mRadius) {
        let cp = new B2ManifoldPoint();
        if (primaryAxis.type === B2EpAxisType.EDGEA) {
          cp.localPoint = b2MulTT2(this.mxf, clipPoints2[i].v);
          cp.id = clipPoints2[i].id;
        } else {
          cp.localPoint = clipPoints2[i].v;
          cp.id.typeA = clipPoints2[i].id.typeB;
          cp.id.typeB = clipPoints2[i].id.typeA;
          cp.id.indexA = clipPoints2[i].id.indexB;
          cp.id.indexB = clipPoints2[i].id.indexA;
        }
        manifold.points.push(cp);
      }
    }
    return manifold;
  }
  public computeEdgeSeparation(): B2EpAxis {
    let axis = new B2EpAxis();
    axis.type = B2EpAxisType.EDGEA;
    axis.index = this.mFront ? 0 : 1;
    axis.separation = b2MaxFloat;
    for (let i = 0; i < this.mPolygonB.count; i++) {
      let s = b2Dot22(this.mNormal, subtract(this.mPolygonB.vertices[i], this.mv1));
      if (s < axis.separation) {
        axis.separation = s;
      }
    }
    return axis;
  }

  public computePolygonSeparation(): B2EpAxis {
    let axis = new B2EpAxis();
    axis.type = B2EpAxisType.UNKNOWN;
    axis.index = -1;
    axis.separation = -b2MaxFloat;
    let perp = new B2Vec2(-this.mNormal.y, this.mNormal.x);
    for (let i = 0; i < this.mPolygonB.count; i++) {
      let n = minus(this.mPolygonB.normals[i]);
      let s1 = b2Dot22(n, subtract(this.mPolygonB.vertices[i], this.mv1));
      let s2 = b2Dot22(n, subtract(this.mPolygonB.vertices[i], this.mv2));
      let s = Math.min(s1, s2);
      if (s > this.mRadius) {
        axis.type = B2EpAxisType.EDGEB;
        axis.index = i;
        axis.separation = s;
        return axis;
      }
      if (b2Dot22(n, perp) >= 0.0) {
        if (b2Dot22(subtract(n, this.mUpperLimit), this.mNormal) < -b2AngularSlop) {
          continue;
        }
      } else {
        if (b2Dot22(subtract(n, this.mLowerLimit), this.mNormal) < -b2AngularSlop) {
          continue;
        }
      }
      if (s > axis.separation) {
        axis.type = B2EpAxisType.EDGEB;
        axis.index = i;
        axis.separation = s;
      }
    }
    return axis;
  }
}

enum VertexType {
  ISOLATED,
  CONCAVE,
  CONVEX
}

function b2CollideEdgeAndPolygon(manifold: B2Manifold, edgeA: B2EdgeShape, xfA: B2Transform, polygonB: B2PolygonShape, xfB: B2Transform): void {
  let collider = new B2EpCollider();
  manifold = collider.collide(edgeA, xfA, polygonB, xfB);
}

export { b2CollideEdgeAndCircle, b2CollideEdgeAndPolygon };
