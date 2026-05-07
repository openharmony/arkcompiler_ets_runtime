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
  b2Cross,
  b2ClampF,
  b2MulT2,
  b2Cross21,
  b2Cross12,
  b2Mul22,
  b2MulR2,
  B2Mat22,
  addEqual,
  multM,
  add,
  subtract,
  subtractEqual,
  minus
} from '../../Common/b2Math';
import { B2Contact } from './b2Contact';
import { B2ManifoldType, B2WorldManifold } from '../../Collision/b2Collision';
import { B2Position, B2Velocity } from '../b2TimeStep';
import { B2Array, B2TimeStep } from '../b2TimeStep';
import {
  b2Baumgarte,
  b2LinearSlop,
  b2MaxLinearCorrection,
  b2MaxManifoldPoints,
  b2ToiBaugarte,
  b2VelocityThreshold,
  half,
  indexTwo
} from '../../Common/b2Settings';

export class B2VelocityConstraintPoint {
  rA = new B2Vec2();
  rB = new B2Vec2();
  normalImpulse: number = 0.0;
  tangentImpulse: number = 0.0;
  normalMass: number = 0.0;
  tangentMass: number = 0.0;
  velocityBias: number = 0.0;
}

export class B2ContactVelocityConstraint {
  points = new Array<B2VelocityConstraintPoint>();
  normal = new B2Vec2();
  normalMass = new B2Mat22();
  mK = new B2Mat22();
  indexA: number = 0;
  indexB: number = 0;
  invMassA: number = 0.0;
  invMassB: number = 0.0;
  invIA: number = 0.0;
  invIB: number = 0.0;
  friction: number = 0.0;
  restitution: number = 0.0;
  tangentSpeed: number = 0.0;
  pointCount: number = 0;
  contactIndex: number = 0;
}

export class B2ContactSolverDef {
  step = new B2TimeStep();
  contacts = new Array<B2Contact>();
  count: number = 0;
  positions = new B2Array<B2Position>();
  velocities = new B2Array<B2Velocity>();
}

export class B2ContactSolver {
  constructor(def: B2ContactSolverDef) {
    this.mStep = def.step;
    this.mCount = def.count;
    this.mPositionConstraints = new Array<B2ContactPositionConstraint>();
    this.mVelocityConstraints = new Array<B2ContactVelocityConstraint>();
    this.mPositions = def.positions;
    this.mVelocities = def.velocities;
    this.mContacts = def.contacts;
    for (let i = 0; i < this.mCount; i++) {
      let contact = this.mContacts[i];
      let fixtureA = contact.mFixtureA;
      let fixtureB = contact.mFixtureB;
      let shapeA = fixtureA?.shape;
      let shapeB = fixtureB?.shape;
      let radiusA = shapeA?.mRadius;
      let radiusB = shapeB?.mRadius;
      let bodyA = fixtureA?.body;
      let bodyB = fixtureB?.body;
      let manifold = contact.manifold;
      let pointCount = manifold.pointCount;
      let vc = new B2ContactVelocityConstraint();
      vc.friction = contact.mFriction;
      vc.restitution = contact.mRestitution;
      vc.tangentSpeed = contact.mTangentSpeed;
      vc.indexA = bodyA?.mIslandIndex!;
      vc.indexB = bodyB?.mIslandIndex!;
      vc.invMassA = bodyA?.mInvMass!;
      vc.invMassB = bodyB?.mInvMass!;
      vc.invIA = bodyA?.mInvI!;
      vc.invIB = bodyB?.mInvI!;
      vc.contactIndex = i;
      vc.pointCount = pointCount;
      vc.mK.setZero();
      vc.normalMass.setZero();
      this.mVelocityConstraints.push(vc);
      let pc = new B2ContactPositionConstraint();
      pc.indexA = bodyA?.mIslandIndex!;
      pc.indexB = bodyB?.mIslandIndex!;
      pc.invMassA = bodyA?.mInvMass!;
      pc.invMassB = bodyB?.mInvMass!;
      pc.localCenterA = bodyA?.mSweep.localCenter!;
      pc.localCenterB = bodyB?.mSweep.localCenter!;
      pc.invIA = bodyA?.mInvI!;
      pc.invIB = bodyB?.mInvI!;
      pc.localNormal = manifold.localNormal;
      pc.localPoint = manifold.localPoint;
      pc.pointCount = pointCount;
      pc.radiusA = radiusA!;
      pc.radiusB = radiusB!;
      pc.type = manifold.type;
      this.mPositionConstraints.push(pc);
      for (let j = 0; j < pointCount; j++) {
        let cp = manifold.points[j];
        let vcp = new B2VelocityConstraintPoint();
        if (this.mStep.warmStarting) {
          vcp.normalImpulse = this.mStep.dtRatio * cp.normalImpulse;
          vcp.tangentImpulse = this.mStep.dtRatio * cp.tangentImpulse;
        } else {
          vcp.normalImpulse = 0.0;
          vcp.tangentImpulse = 0.0;
        }
        vcp.rA.setZero();
        vcp.rB.setZero();
        vcp.normalMass = 0.0;
        vcp.tangentMass = 0.0;
        vcp.velocityBias = 0.0;
        vc.points.push(vcp);
        pc.localPoints[j] = cp.localPoint;
      }
    }
  }
  
  initializeVelocityConstraints(): void {
    for (let i = 0; i < this.mCount; i++) {
      let vc = this.mVelocityConstraints[i];
      let pc = this.mPositionConstraints[i];
      let radiusA = pc.radiusA;
      let radiusB = pc.radiusB;
      let manifold = this.mContacts[vc.contactIndex].manifold;
      let indexA = vc.indexA;
      let indexB = vc.indexB;
      let mA = vc.invMassA;
      let mB = vc.invMassB;
      let iA = vc.invIA;
      let iB = vc.invIB;
      let localCenterA = pc.localCenterA;
      let localCenterB = pc.localCenterB;
      let cA: B2Vec2 = this.mPositions.get(indexA).c;
      let aA: number = this.mPositions.get(indexA).a;
      let vA: B2Vec2 = this.mVelocities.get(indexA).v;
      let wA: number = this.mVelocities.get(indexA).w;
      let cB: B2Vec2 = this.mPositions.get(indexB).c;
      let aB: number = this.mPositions.get(indexB).a;
      let vB: B2Vec2 = this.mVelocities.get(indexB).v;
      let wB: number = this.mVelocities.get(indexB).w;
      let xfA = new B2Transform();
      let xfB = new B2Transform();
      xfA.q.set(aA);
      xfB.q.set(aB);
      xfA.p = subtract(cA, b2MulR2(xfA.q, localCenterA));
      xfB.p = subtract(cB, b2MulR2(xfB.q, localCenterB));
      let worldManifold = new B2WorldManifold();
      worldManifold.initialize(manifold, xfA, radiusA, xfB, radiusB);
      vc.normal = worldManifold.normal;
      let pointCount = vc.pointCount;
      for (let j = 0; j < pointCount; j++) {
        let vcp = vc.points[j];
        vcp.rA = subtract(worldManifold.points[j], cA);
        vcp.rB = subtract(worldManifold.points[j], cB);
        let rnA = b2Cross(vcp.rA, vc.normal);
        let rnB = b2Cross(vcp.rB, vc.normal);
        let kNormal = mA + mB + iA * rnA * rnA + iB * rnB * rnB;
        vcp.normalMass = kNormal > 0.0 ? 1.0 / kNormal : 0.0;
        let tangent = b2Cross12(vc.normal, 1.0);
        let rtA = b2Cross(vcp.rA, tangent);
        let rtB = b2Cross(vcp.rB, tangent);
        let kTangent = mA + mB + iA * rtA * rtA + iB * rtB * rtB;
        vcp.tangentMass = kTangent > 0.0 ? 1.0 / kTangent : 0.0;
        vcp.velocityBias = 0.0;
        let vRel = b2Dot22(vc.normal, subtract(subtract(add(vB, b2Cross21(wB, vcp.rB)), vA), b2Cross21(wA, vcp.rA)));
        if (vRel < -b2VelocityThreshold) {
          vcp.velocityBias = -vc.restitution * vRel;
        }
      }
      if (vc.pointCount === indexTwo) {
        let vcp1 = vc.points[0];
        let vcp2 = vc.points[1];
        let rn1A = b2Cross(vcp1.rA, vc.normal);
        let rn1B = b2Cross(vcp1.rB, vc.normal);
        let rn2A = b2Cross(vcp2.rA, vc.normal);
        let rn2B = b2Cross(vcp2.rB, vc.normal);
        let k11 = mA + mB + iA * rn1A * rn1A + iB * rn1B * rn1B;
        let k22 = mA + mB + iA * rn2A * rn2A + iB * rn2B * rn2B;
        let k12 = mA + mB + iA * rn1A * rn2A + iB * rn1B * rn2B;
        let kMaxConditionNumber: number = 1000.0;
        if (k11 * k11 < kMaxConditionNumber * (k11 * k22 - k12 * k12)) {
          vc.mK.ex.set(k11, k12);
          vc.mK.ey.set(k12, k22);
          vc.normalMass = vc.mK.getInverse();
        } else {
          vc.pointCount = 1;
        }
      }
    }
  }
  
  warmStart(): void {
    for (let i = 0; i < this.mCount; i++) {
      let vc = this.mVelocityConstraints[i];
      let indexA = vc.indexA;
      let indexB = vc.indexB;
      let mA = vc.invMassA;
      let iA = vc.invIA;
      let mB = vc.invMassB;
      let iB = vc.invIB;
      let pointCount = vc.pointCount;
      let vA: B2Vec2 = this.mVelocities.get(indexA).v;
      let wA: number = this.mVelocities.get(indexA).w;
      let vB: B2Vec2 = this.mVelocities.get(indexB).v;
      let wB: number = this.mVelocities.get(indexB).w;
      let normal = vc.normal;
      let tangent = b2Cross12(normal, 1.0);
      for (let j = 0; j < pointCount; j++) {
        let vcp = vc.points[j];
        let mP = add(multM(normal, vcp.normalImpulse), multM(tangent, vcp.tangentImpulse));
        wA -= iA * b2Cross(vcp.rA, mP);
        subtractEqual(vA, multM(mP, mA));
        wB += iB * b2Cross(vcp.rB, mP);
        addEqual(vB, multM(mP, mB));
      }
      this.mVelocities.get(indexA).v = vA;
      this.mVelocities.get(indexA).w = wA;
      this.mVelocities.get(indexB).v = vB;
      this.mVelocities.get(indexB).w = wB;
    }
  }
  
  solveVelocityConstraints(): void {
    for (let i = 0; i < this.mCount; i++) {
      let vc = this.mVelocityConstraints[i];
      let indexA = vc.indexA;
      let indexB = vc.indexB;
      let mA = vc.invMassA;
      let iA = vc.invIA;
      let mB = vc.invMassB;
      let iB = vc.invIB;
      let pointCount = vc.pointCount;
      let vA: B2Vec2 = this.mVelocities.get(indexA).v;
      let wA: number = this.mVelocities.get(indexA).w;
      let vB: B2Vec2 = this.mVelocities.get(indexB).v;
      let wB: number = this.mVelocities.get(indexB).w;
      let normal = vc.normal;
      let tangent = b2Cross12(normal, 1.0);
      let friction = vc.friction;
      for (let j = 0; j < pointCount; j++) {
        let vcp = vc.points[j];
        let dv = subtract(subtract(add(vB, b2Cross21(wB, vcp.rB)), vA), b2Cross21(wA, vcp.rA));
        let vt = b2Dot22(dv, tangent) - vc.tangentSpeed;
        let lambda = vcp.tangentMass * -vt;
        let maxFriction = friction * vcp.normalImpulse;
        let newImpulse = b2ClampF(vcp.tangentImpulse + lambda, -maxFriction, maxFriction);
        lambda = newImpulse - vcp.tangentImpulse;
        vcp.tangentImpulse = newImpulse;
        let mP = multM(tangent, lambda);
        subtractEqual(vA, multM(mP, mA));
        wA -= iA * b2Cross(vcp.rA, mP);
        addEqual(vB, multM(mP, mB));
        wB += iB * b2Cross(vcp.rB, mP);
      }
      if (vc.pointCount === 1) {
        let vcp = vc.points[0];
        let dv = subtract(subtract(add(vB, b2Cross21(wB, vcp.rB)), vA), b2Cross21(wA, vcp.rA));
        let vn = b2Dot22(dv, normal);
        let lambda = -vcp.normalMass * (vn - vcp.velocityBias);
        let newImpulse = Math.max(vcp.normalImpulse + lambda, 0.0);
        lambda = newImpulse - vcp.normalImpulse;
        vcp.normalImpulse = newImpulse;
        let mP = multM(normal, lambda);
        subtractEqual(vA, multM(mP, mA));
        wA -= iA * b2Cross(vcp.rA, mP);
        addEqual(vB, multM(mP, mB));
        wB += iB * b2Cross(vcp.rB, mP);
      } else {
        let cp1 = vc.points[0];
        let cp2 = vc.points[1];
        let a = new B2Vec2(cp1.normalImpulse, cp2.normalImpulse);
        let dv1 = subtract(subtract(add(vB, b2Cross21(wB, cp1.rB)), vA), b2Cross21(wA, cp1.rA));
        let dv2 = subtract(subtract(add(vB, b2Cross21(wB, cp2.rB)), vA), b2Cross21(wA, cp2.rA));
        let vn1 = b2Dot22(dv1, normal);
        let vn2 = b2Dot22(dv2, normal);
        let b = new B2Vec2();
        b.x = vn1 - cp1.velocityBias;
        b.y = vn2 - cp2.velocityBias;
        subtractEqual(b, b2Mul22(vc.mK, a));
        while (true) {
          let x = minus(b2Mul22(vc.normalMass, b));
          if (x.x >= 0.0 && x.y >= 0.0) {
            let d = subtract(x, a);
            let mP1 = multM(normal, d.x);
            let mP2 = multM(normal, d.y);
            subtractEqual(vA, multM(add(mP1, mP2), mA));
            wA -= iA * (b2Cross(cp1.rA, mP1) + b2Cross(cp2.rA, mP2));
            addEqual(vB, multM(add(mP1, mP2), mB));
            wB += iB * (b2Cross(cp1.rB, mP1) + b2Cross(cp2.rB, mP2));
            cp1.normalImpulse = x.x;
            cp2.normalImpulse = x.y;
            dv1 = subtract(subtract(add(vB, b2Cross21(wB, cp1.rB)), vA), b2Cross21(wA, cp1.rA));
            dv2 = subtract(subtract(add(vB, b2Cross21(wB, cp2.rB)), vA), b2Cross21(wA, cp2.rA));
            vn1 = b2Dot22(dv1, normal);
            vn2 = b2Dot22(dv2, normal);
            break;
          }
          x.x = -cp1.normalMass * b.x;
          x.y = 0.0;
          vn1 = 0.0;
          vn2 = vc.mK.ex.y * x.x + b.y;
          if (x.x >= 0.0 && vn2 >= 0.0) {
            let d = subtract(x, a);
            let mP1 = multM(normal, d.x);
            let mP2 = multM(normal, d.y);
            subtractEqual(vA, multM(add(mP1, mP2), mA));
            wA -= iA * (b2Cross(cp1.rA, mP1) + b2Cross(cp2.rA, mP2));
            addEqual(vB, multM(add(mP1, mP2), mB));
            wB += iB * (b2Cross(cp1.rB, mP1) + b2Cross(cp2.rB, mP2));
            cp1.normalImpulse = x.x;
            cp2.normalImpulse = x.y;
            dv1 = subtract(subtract(add(vB, b2Cross21(wB, cp1.rB)), vA), b2Cross21(wA, cp1.rA));
            vn1 = b2Dot22(dv1, normal);
            break;
          }
          x.x = 0.0;
          x.y = -cp2.normalMass * b.y;
          vn1 = vc.mK.ey.x * x.y + b.x;
          vn2 = 0.0;
          if (x.y >= 0.0 && vn1 >= 0.0) {
            let d = subtract(x, a);
            let mP1 = multM(normal, d.x);
            let mP2 = multM(normal, d.y);
            subtractEqual(vA, multM(add(mP1, mP2), mA));
            wA -= iA * (b2Cross(cp1.rA, mP1) + b2Cross(cp2.rA, mP2));
            addEqual(vB, multM(add(mP1, mP2), mB));
            wB += iB * (b2Cross(cp1.rB, mP1) + b2Cross(cp2.rB, mP2));
            cp1.normalImpulse = x.x;
            cp2.normalImpulse = x.y;
            dv2 = subtract(subtract(add(vB, b2Cross21(wB, cp2.rB)), vA), b2Cross21(wA, cp2.rA));
            vn2 = b2Dot22(dv2, normal);
            break;
          }
          x.x = 0.0;
          x.y = 0.0;
          vn1 = b.x;
          vn2 = b.y;
          if (vn1 >= 0.0 && vn2 >= 0.0) {
            let d = subtract(x, a);
            let mP1 = multM(normal, d.x);
            let mP2 = multM(normal, d.y);
            subtractEqual(vA, multM(add(mP1, mP2), mA));
            wA -= iA * (b2Cross(cp1.rA, mP1) + b2Cross(cp2.rA, mP2));
            addEqual(vB, multM(add(mP1, mP2), mB));
            wB += iB * (b2Cross(cp1.rB, mP1) + b2Cross(cp2.rB, mP2));
            cp1.normalImpulse = x.x;
            cp2.normalImpulse = x.y;
            break;
          }
          break;
        }
      }
      this.mVelocities.get(indexA).v = vA;
      this.mVelocities.get(indexA).w = wA;
      this.mVelocities.get(indexB).v = vB;
      this.mVelocities.get(indexB).w = wB;
    }
  }
  
  storeImpulses(): void {
    for (let i = 0; i < this.mCount; i++) {
      let vc = this.mVelocityConstraints[i];
      let manifold = this.mContacts[vc.contactIndex].manifold;
      for (let j = 0; j < vc.pointCount; j++) {
        manifold.points[j].normalImpulse = vc.points[j].normalImpulse;
        manifold.points[j].tangentImpulse = vc.points[j].tangentImpulse;
      }
    }
  }
  
  solvePositionConstraints(): boolean {
    let minSeparation: number = 0.0;
    let ratio: number = -3.0;
    for (let i = 0; i < this.mCount; i++) {
      let pc = this.mPositionConstraints[i];
      let indexA = pc.indexA;
      let indexB = pc.indexB;
      let localCenterA = pc.localCenterA;
      let mA = pc.invMassA;
      let iA = pc.invIA;
      let localCenterB = pc.localCenterB;
      let mB = pc.invMassB;
      let iB = pc.invIB;
      let pointCount = pc.pointCount;
      let cA: B2Vec2 = this.mPositions.get(indexA).c;
      let aA: number = this.mPositions.get(indexA).a;
      let cB: B2Vec2 = this.mPositions.get(indexB).c;
      let aB: number = this.mPositions.get(indexB).a;
      for (let j = 0; j < pointCount; j++) {
        let xfA = new B2Transform();
        let xfB = new B2Transform();
        xfA.q.set(aA);
        xfB.q.set(aB);
        xfA.p = subtract(cA, b2MulR2(xfA.q, localCenterA));
        xfB.p = subtract(cB, b2MulR2(xfB.q, localCenterB));
        let psm = new B2PositionSolverManifold();
        psm.initialize(pc, xfA, xfB, j);
        let normal = psm.normal;
        let point = psm.point;
        let separation = psm.separation;
        let rA = subtract(point, cA);
        let rB = subtract(point, cB);
        minSeparation = Math.min(minSeparation, separation);
        let mC = b2ClampF(b2Baumgarte * (separation + b2LinearSlop), -b2MaxLinearCorrection, 0.0);
        let rnA = b2Cross(rA, normal);
        let rnB = b2Cross(rB, normal);
        let mK = mA + mB + iA * rnA * rnA + iB * rnB * rnB;
        let impulse = mK > 0.0 ? -mC / mK : 0.0;
        let mP = multM(normal, impulse);
        subtractEqual(cA, multM(mP, mA));
        aA -= iA * b2Cross(rA, mP);
        addEqual(cB, multM(mP, mB));
        aB += iB * b2Cross(rB, mP);
      }
      this.mPositions.get(indexA).c = cA;
      this.mPositions.get(indexA).a = aA;
      this.mPositions.get(indexB).c = cB;
      this.mPositions.get(indexB).a = aB;
    }
    return minSeparation >= ratio * b2LinearSlop;
  }

  solveToiPositionConstraints(toiIndexA: number, toiIndexB: number): boolean {
    let ratio: number = -1.5;
    let minSeparation: number = 0.0;
    for (let i = 0; i < this.mCount; i++) {
      let pc = this.mPositionConstraints[i];
      let indexA = pc.indexA;
      let indexB = pc.indexB;
      let localCenterA = pc.localCenterA;
      let localCenterB = pc.localCenterB;
      let pointCount = pc.pointCount;
      let mA: number = 0.0;
      let iA: number = 0.0;
      if (indexA === toiIndexA || indexA === toiIndexB) {
        mA = pc.invMassA;
        iA = pc.invIA;
      }
      let mB: number = 0.0;
      let iB: number = 0.0;
      if (indexB === toiIndexA || indexB === toiIndexB) {
        mB = pc.invMassB;
        iB = pc.invIB;
      }
      let cA: B2Vec2 = this.mPositions.get(indexA).c;
      let aA: number = this.mPositions.get(indexA).a;
      let cB: B2Vec2 = this.mPositions.get(indexB).c;
      let aB: number = this.mPositions.get(indexB).a;
      for (let j = 0; j < pointCount; j++) {
        let xfA = new B2Transform();
        let xfB = new B2Transform();
        xfA.q.set(aA);
        xfB.q.set(aB);
        xfA.p = subtract(cA, b2MulR2(xfA.q, localCenterA));
        xfB.p = subtract(cB, b2MulR2(xfB.q, localCenterB));
        let psm = new B2PositionSolverManifold();
        psm.initialize(pc, xfA, xfB, j);
        let normal = psm.normal;
        let point = psm.point;
        let separation = psm.separation;
        let rA = subtract(point, cA);
        let rB = subtract(point, cB);
        minSeparation = Math.min(minSeparation, separation);
        let mC = b2ClampF(b2ToiBaugarte * (separation + b2LinearSlop), -b2MaxLinearCorrection, 0.0);
        let rnA = b2Cross(rA, normal);
        let rnB = b2Cross(rB, normal);
        let mK = mA + mB + iA * rnA * rnA + iB * rnB * rnB;
        let impulse = mK > 0.0 ? -mC / mK : 0.0;
        let mP = multM(normal, impulse);
        subtractEqual(cA, multM(mP, mA));
        aA -= iA * b2Cross(rA, mP);
        addEqual(cB, multM(mP, mB));
        aB += iB * b2Cross(rB, mP);
      }
      this.mPositions.get(indexA).c = cA;
      this.mPositions.get(indexA).a = aA;
      this.mPositions.get(indexB).c = cB;
      this.mPositions.get(indexB).a = aB;
    }
    return minSeparation >= ratio * b2LinearSlop;
  }
  
  mStep: B2TimeStep;
  mPositions: B2Array<B2Position>;
  mVelocities: B2Array<B2Velocity>;
  mPositionConstraints: Array<B2ContactPositionConstraint>;
  mVelocityConstraints: Array<B2ContactVelocityConstraint>;
  mContacts: Array<B2Contact>;
  mCount: number;
}

export class B2ContactPositionConstraint {
  localPoints: B2Vec2[];
  localNormal = new B2Vec2();
  localPoint = new B2Vec2();
  indexA = 0;
  indexB = 0;
  invMassA: number = 0.0;
  invMassB: number = 0.0;
  localCenterA = new B2Vec2();
  localCenterB = new B2Vec2();
  invIA: number = 0;
  invIB: number = 0;
  type: B2ManifoldType = B2ManifoldType.CIRCLES;
  radiusA: number = 0;
  radiusB: number = 0;
  pointCount = 0;

  constructor() {
    this.localPoints = [];
    for (let i = 0; i < b2MaxManifoldPoints; i++) {
      this.localPoints.push(new B2Vec2());
    }
  }
}

export class B2PositionSolverManifold {
  initialize(pc: B2ContactPositionConstraint, xfA: B2Transform, xfB: B2Transform, index: number): void {
    switch (pc.type) {
      case B2ManifoldType.CIRCLES: {
        let pointA = b2MulT2(xfA, pc.localPoint);
        let pointB = b2MulT2(xfB, pc.localPoints[0]);
        this.normal = subtract(pointB, pointA);
        this.normal.normalize();
        this.point = multM(add(pointA, pointB), half);
        this.separation = b2Dot22(subtract(pointB, pointA), this.normal) - pc.radiusA - pc.radiusB;
        break;
      }
      case B2ManifoldType.FACEA: {
        this.normal = b2MulR2(xfA.q, pc.localNormal);
        let planePoint0 = b2MulT2(xfA, pc.localPoint);
        let clipPoint0 = b2MulT2(xfB, pc.localPoints[index]);
        this.separation = b2Dot22(subtract(clipPoint0, planePoint0), this.normal) - pc.radiusA - pc.radiusB;
        this.point = clipPoint0;
        break;
      }
      case B2ManifoldType.FACEB: {
        this.normal = b2MulR2(xfB.q, pc.localNormal);
        let planePoint = b2MulT2(xfB, pc.localPoint);
        let clipPoint = b2MulT2(xfA, pc.localPoints[index]);
        this.separation = b2Dot22(subtract(clipPoint, planePoint), this.normal) - pc.radiusA - pc.radiusB;
        this.point = clipPoint;
        this.normal = minus(this.normal);
        break;
      }
      default:
        break;
    }
  }
  normal = new B2Vec2();
  point = new B2Vec2();
  separation: number = 0.0;
}
