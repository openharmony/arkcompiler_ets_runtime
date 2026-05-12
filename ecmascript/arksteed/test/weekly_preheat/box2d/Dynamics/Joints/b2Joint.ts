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
import { B2Body } from '../b2Body';
import {
  add,
  addEqual,
  addEqual3,
  b2ClampF,
  b2Cross,
  b2Cross21,
  b2Dot22,
  B2Mat22,
  B2Mat33,
  b2Mul22,
  b2Mul32,
  b2Mul33,
  b2MulR2,
  b2MulTR2,
  b2MulTT2,
  B2Rot,
  B2Vec2,
  B2Vec3,
  minus,
  minus3,
  mulEqual,
  mulMEqual3,
  multM,
  subtract,
  subtract3,
  subtractEqual
} from '../../Common/b2Math';
import { B2SolverData } from '../b2TimeStep';
import { b2AngularSlop, b2LinearSlop, b2MaxAngularCorrection, b2MaxLinearCorrection, b2PI, float10, float2 } from '../../Common/b2Settings';

enum B2JointType {
  UNKNOWNJOINT,
  REVOLUTEJOINT,
  PRISMATICJOINT,
  DISTANCEJOINT,
  PULLEYJOINT,
  MOUSEJOINT,
  GEARJOINT,
  WHEELJOINT,
  WELDJOINT,
  FRICTIONJOINT,
  ROPEJOINT,
  MOTORJOINT
}

enum B2LimitState {
  INACTIVELIMIT,
  ATLOWERLIMIT,
  ATUPPERLIMIT,
  EQUALLIMITS
}

export class B2JointEdge {
  constructor(joint: B2Joint) {
    this.joint = joint;
  }
  other: B2Body | null = null;
  joint: B2Joint;
  prev: B2JointEdge | null = null;
  next: B2JointEdge | null = null;
}

export class B2JointDef {
  constructor() {
    this.type = B2JointType.UNKNOWNJOINT;
    this.userData = null;
    this.bodyA = null;
    this.bodyB = null;
    this.collideConnected = false;
  }
  public type: B2JointType;
  public userData: Object | null;
  public bodyA: B2Body | null;
  public bodyB: B2Body | null;
  public collideConnected: boolean;
}

export class B2Joint {
  mType: B2JointType = B2JointType.UNKNOWNJOINT;
  mPrev: B2Joint | null = null;
  mNext: B2Joint | null = null;
  mEdgeA: B2JointEdge;
  mEdgeB: B2JointEdge;
  mBodyA: B2Body | null;
  mBodyB: B2Body | null;
  mIndex: number = 0;
  mIslandFlag: boolean = false;
  mCollideConnected: boolean = false;
  mUserData: Object | null = null;
  public get typeJoint(): B2JointType {
    return this.mType;
  }
  public get bodyA(): B2Body | null {
    return this.mBodyA;
  }
  public get bodyB(): B2Body | null {
    return this.mBodyB;
  }
  public get anchorA(): B2Vec2 {
    throw new Error('must override');
  }
  public get anchorB(): B2Vec2 {
    throw new Error('must override');
  }
  constructor(def: B2JointDef) {
    this.mType = def.type;
    this.mPrev = null;
    this.mNext = null;
    this.mBodyA = def.bodyA;
    this.mBodyB = def.bodyB;
    this.mIndex = 0;
    this.mCollideConnected = def.collideConnected;
    this.mIslandFlag = false;
    this.mUserData = def.userData;
    this.mEdgeA = new B2JointEdge(this);
    this.mEdgeA.other = null;
    this.mEdgeA.prev = null;
    this.mEdgeA.next = null;
    this.mEdgeB = new B2JointEdge(this);
    this.mEdgeB.other = null;
    this.mEdgeB.prev = null;
    this.mEdgeB.next = null;
  }
  public getReactionForce(invdt: number): B2Vec2 {
    throw new Error('must override');
  }
  public getReactionTorque(invdt: number): number {
    throw new Error('must override');
  }
  public getNext(): B2Joint | null {
    return this.mNext;
  }
  public get userData(): Object | null {
    return this.mUserData;
  }
  public set userData(value: Object | null) {
    this.setUserData(value);
  }
  setUserData(data: Object | null): void {
    this.mUserData = data;
  }
  get isActive(): boolean {
    return this.mBodyA!.isActive && this.mBodyB!.isActive;
  }
  collideConnected(): boolean {
    return this.mCollideConnected;
  }
  public shiftOrigin(newOrigin: B2Vec2): void {}
  static create(def: B2JointDef): B2Joint {
    let joint: B2Joint;
    switch (def.type) {
      case B2JointType.DISTANCEJOINT:
        joint = new B2DistanceJoint(def as B2DistanceJointDef);
        break;
      case B2JointType.MOUSEJOINT:
        joint = new B2MouseJoint(def as B2MouseJointDef);
        break;
      case B2JointType.PRISMATICJOINT:
        joint = new B2PrismaticJoint(def as B2PrismaticJointDef);
        break;
      case B2JointType.REVOLUTEJOINT:
        joint = new B2RevoluteJoint(def as B2RevoluteJointDef);
        break;
      case B2JointType.PULLEYJOINT:
        joint = new B2PulleyJoint(def as B2PulleyJointDef);
        break;
      case B2JointType.GEARJOINT:
        joint = new B2GearJoint(def as B2GearJointDef);
        break;
      case B2JointType.WELDJOINT:
        joint = new B2WeldJoint(def as B2WeldJointDef);
        break;
      case B2JointType.FRICTIONJOINT:
        joint = new B2FrictionJoint(def as B2FrictionJointDef);
        break;
      default:
        throw new Error('Unknown joint type');
    }
    return joint;
  }
  static destroy(joint: B2Joint): void {}
  initVelocityConstraints(data: B2SolverData): void {
    throw new Error('must override');
  }
  solveVelocityConstraints(data: B2SolverData): void {
    throw new Error('must override');
  }
  solvePositionConstraints(data: B2SolverData): boolean {
    throw new Error('must override');
  }
}

export class B2WeldJointDef extends B2JointDef {
  public constructor() {
    super();
    this.localAnchorA = new B2Vec2(0.0, 0.0);
    this.localAnchorB = new B2Vec2(0.0, 0.0);
    this.referenceAngle = 0.0;
    this.frequencyHz = 0.0;
    this.dampingRatio = 0.0;
    this.type = B2JointType.WELDJOINT;
  }
  
  public initialize(bodyA: B2Body, bodyB: B2Body, anchor: B2Vec2): void {
    this.bodyA = bodyA;
    this.bodyB = bodyB;
    this.localAnchorA = bodyA.getLocalPoint(anchor);
    this.localAnchorB = bodyB.getLocalPoint(anchor);
    this.referenceAngle = bodyB.angle - bodyA.angle;
  }
  public localAnchorA: B2Vec2;
  public localAnchorB: B2Vec2;
  public referenceAngle: number;
  public frequencyHz: number;
  public dampingRatio: number;
}

export class B2WeldJoint extends B2Joint {
  mFrequencyHz: number = 0.0;
  mDampingRatio: number = 0.0;
  mBias: number = 0.0;
  mLocalAnchorA = new B2Vec2();
  mLocalAnchorB = new B2Vec2();
  mReferenceAngle: number = 0.0;
  mGamma: number = 0.0;
  mImpulse = new B2Vec3();
  mIndexA: number = 0;
  mIndexB: number = 0;
  mrA = new B2Vec2();
  mrB = new B2Vec2();
  mLocalCenterA = new B2Vec2();
  mLocalCenterB = new B2Vec2();
  mInvMassA: number = 0.0;
  mInvMassB: number = 0.0;
  mInvIA: number = 0.0;
  mInvIB: number = 0.0;
  mMass = new B2Mat33();
  public get anchorA(): B2Vec2 {
    return this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
  }
  public get anchorB(): B2Vec2 {
    return this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
  }
  public getReactionForce(invdt: number): B2Vec2 {
    let mP = new B2Vec2(this.mImpulse.x, this.mImpulse.y);
    return multM(mP, invdt);
  }
  public getReactionTorque(invdt: number): number {
    return invdt * this.mImpulse.z;
  }
  public get localAnchorA(): B2Vec2 {
    return this.mLocalAnchorA;
  }
  public get localAnchorB(): B2Vec2 {
    return this.mLocalAnchorB;
  }
  public get referenceAngle(): number {
    return this.mReferenceAngle;
  }
  public setFrequency(hz: number): void {
    this.mFrequencyHz = hz;
  }
  public get frequency(): number {
    return this.mFrequencyHz;
  }
  public setDampingRatio(ratio: number): void {
    this.mDampingRatio = ratio;
  }
  public get dampingRatio(): number {
    return this.mDampingRatio;
  }
  constructor(def: B2WeldJointDef) {
    super(def);
    this.mLocalAnchorA = def.localAnchorA;
    this.mLocalAnchorB = def.localAnchorB;
    this.mReferenceAngle = def.referenceAngle;
    this.mFrequencyHz = def.frequencyHz;
    this.mDampingRatio = def.dampingRatio;
    this.mImpulse = new B2Vec3(0.0, 0.0, 0.0);
  }
  
  initVelocityConstraints(data: B2SolverData): void {
    this.mIndexA = this.mBodyA!.mIslandIndex;
    this.mIndexB = this.mBodyB!.mIslandIndex;
    this.mLocalCenterA = this.mBodyA!.mSweep.localCenter;
    this.mLocalCenterB = this.mBodyB!.mSweep.localCenter;
    this.mInvMassA = this.mBodyA!.mInvMass;
    this.mInvMassB = this.mBodyB!.mInvMass;
    this.mInvIA = this.mBodyA!.mInvI;
    this.mInvIB = this.mBodyB!.mInvI;
    let aA: number = data.positions.get(this.mIndexA).a;
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let aB: number = data.positions.get(this.mIndexB).a;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    this.mrA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    this.mrB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    let mat = new B2Mat33();
    mat.ex.x = mA + mB + this.mrA.y * this.mrA.y * iA + this.mrB.y * this.mrB.y * iB;
    mat.ey.x = -this.mrA.y * this.mrA.x * iA - this.mrB.y * this.mrB.x * iB;
    mat.ez.x = -this.mrA.y * iA - this.mrB.y * iB;
    mat.ex.y = mat.ey.x;
    mat.ey.y = mA + mB + this.mrA.x * this.mrA.x * iA + this.mrB.x * this.mrB.x * iB;
    mat.ez.y = this.mrA.x * iA + this.mrB.x * iB;
    mat.ex.z = mat.ez.x;
    mat.ey.z = mat.ez.y;
    mat.ez.z = iA + iB;
    if (this.mFrequencyHz > 0.0) {
      this.mMass = mat.getInverse22();
      let invM = iA + iB;
      let m = invM > 0.0 ? 1.0 / invM : 0.0;
      let c = aB - aA - this.mReferenceAngle;
      let omega = float2 * b2PI * this.mFrequencyHz;
      let d = float2 * m * this.mDampingRatio * omega;
      let k = m * omega * omega;
      let h = data.step.dt;
      this.mGamma = h * (d + h * k);
      this.mGamma = this.mGamma !== 0.0 ? 1.0 / this.mGamma : 0.0;
      this.mBias = c * h * k * this.mGamma;
      invM += this.mGamma;
      this.mMass.ez.z = invM !== 0.0 ? 1.0 / invM : 0.0;
    } else {
      this.mMass = mat.getSymInverse33();
      this.mGamma = 0.0;
      this.mBias = 0.0;
    }
    if (data.step.warmStarting) {
      mulMEqual3(this.mImpulse, data.step.dtRatio);
      let mP = new B2Vec2(this.mImpulse.x, this.mImpulse.y);
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * (b2Cross(this.mrA, mP) + this.mImpulse.z);
      addEqual(vB, multM(mP, mB));
      wB += iB * (b2Cross(this.mrB, mP) + this.mImpulse.z);
    } else {
      this.mImpulse.setZero();
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solveVelocityConstraints(data: B2SolverData): void {
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    if (this.mFrequencyHz > 0.0) {
      let cdot2 = wB - wA;
      let impulse2 = -this.mMass.ez.z * (cdot2 + this.mBias + this.mGamma * this.mImpulse.z);
      this.mImpulse.z += impulse2;
      wA -= iA * impulse2;
      wB += iB * impulse2;
      let cdot1 = subtract(subtract(add(vB, b2Cross21(wB, this.mrB)), vA), b2Cross21(wA, this.mrA));
      let impulse1 = minus(b2Mul32(this.mMass, cdot1));
      this.mImpulse.x += impulse1.x;
      this.mImpulse.y += impulse1.y;
      let mP = impulse1;
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * b2Cross(this.mrA, mP);
      addEqual(vB, multM(mP, mB));
      wB += iB * b2Cross(this.mrB, mP);
    } else {
      let cdot1 = subtract(subtract(add(vB, b2Cross21(wB, this.mrB)), vA), b2Cross21(wA, this.mrA));
      let cdot2 = wB - wA;
      let cdot = new B2Vec3(cdot1.x, cdot1.y, cdot2);
      let impulse = minus3(b2Mul33(this.mMass, cdot));
      addEqual3(this.mImpulse, impulse);
      let mP = new B2Vec2(impulse.x, impulse.y);
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * (b2Cross(this.mrA, mP) + impulse.z);
      addEqual(vB, multM(mP, mB));
      wB += iB * (b2Cross(this.mrB, mP) + impulse.z);
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solvePositionConstraints(data: B2SolverData): boolean {
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    let rA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    let rB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let positionError: number;
    let angularError: number;
    let mK = new B2Mat33();
    mK.ex.x = mA + mB + rA.y * rA.y * iA + rB.y * rB.y * iB;
    mK.ey.x = -rA.y * rA.x * iA - rB.y * rB.x * iB;
    mK.ez.x = -rA.y * iA - rB.y * iB;
    mK.ex.y = mK.ey.x;
    mK.ey.y = mA + mB + rA.x * rA.x * iA + rB.x * rB.x * iB;
    mK.ez.y = rA.x * iA + rB.x * iB;
    mK.ex.z = mK.ez.x;
    mK.ey.z = mK.ez.y;
    mK.ez.z = iA + iB;
    if (this.mFrequencyHz > 0.0) {
      let mC1 = subtract(subtract(add(cB, rB), cA), rA);
      positionError = mC1.length();
      angularError = 0.0;
      let mP = minus(mK.solve22(mC1));
      subtractEqual(cA, multM(mP, mA));
      aA -= iA * b2Cross(rA, mP);
      addEqual(cB, multM(mP, mB));
      aB += iB * b2Cross(rB, mP);
    } else {
      let mC1 = subtract(subtract(add(cB, rB), cA), rA);
      let mC2 = aB - aA - this.mReferenceAngle;
      positionError = mC1.length();
      angularError = Math.abs(mC2);
      let mC = new B2Vec3(mC1.x, mC1.y, mC2);
      let impulse = minus3(mK.solve33(mC));
      let mP = new B2Vec2(impulse.x, impulse.y);
      subtractEqual(cA, multM(mP, mA));
      aA -= iA * (b2Cross(rA, mP) + impulse.z);
      addEqual(cB, multM(mP, mB));
      aB += iB * (b2Cross(rB, mP) + impulse.z);
    }
    data.positions.get(this.mIndexA).c = cA;
    data.positions.get(this.mIndexA).a = aA;
    data.positions.get(this.mIndexB).c = cB;
    data.positions.get(this.mIndexB).a = aB;
    return positionError <= b2LinearSlop && angularError <= b2AngularSlop;
  }
}

export class B2RevoluteJointDef extends B2JointDef {
  public constructor() {
    super();
    this.localAnchorA = new B2Vec2(0.0, 0.0);
    this.localAnchorB = new B2Vec2(0.0, 0.0);
    this.referenceAngle = 0.0;
    this.lowerAngle = 0.0;
    this.upperAngle = 0.0;
    this.maxMotorTorque = 0.0;
    this.motorSpeed = 0.0;
    this.enableLimit = false;
    this.enableMotor = false;
    this.type = B2JointType.REVOLUTEJOINT;
  }

  public initialize(bodyA: B2Body, bodyB: B2Body, anchor: B2Vec2): void {
    this.bodyA = bodyA;
    this.bodyB = bodyB;
    this.localAnchorA = bodyA.getLocalPoint(anchor);
    this.localAnchorB = bodyB.getLocalPoint(anchor);
    this.referenceAngle = bodyB.angle - bodyA.angle;
  }
  public localAnchorA: B2Vec2;
  public localAnchorB: B2Vec2;
  public referenceAngle: number;
  public maxMotorTorque: number;
  public motorSpeed: number;
  public lowerAngle: number;
  public upperAngle: number;
  public enableLimit: boolean;
  public enableMotor: boolean;
}

export class B2RevoluteJoint extends B2Joint {
  public get anchorA(): B2Vec2 {
    return this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
  }
  public get anchorB(): B2Vec2 {
    return this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
  }
  public get localAnchorA(): B2Vec2 {
    return this.mLocalAnchorA;
  }
  public get localAnchorB(): B2Vec2 {
    return this.mLocalAnchorB;
  }
  public get referenceAngle(): number {
    return this.mReferenceAngle;
  }
  public get jointAngle(): number {
    let bA = this.mBodyA;
    let bB = this.mBodyB;
    return bB!.mSweep.a - bA!.mSweep.a - this.mReferenceAngle;
  }
  
  public get jointSpeed(): number {
    let bA = this.mBodyA;
    let bB = this.mBodyB;
    return bB!.mAngularVelocity - bA!.mAngularVelocity;
  }
  
  public get isLimitEnabled(): boolean {
    return this.mEnableLimit;
  }
  public set isLimitEnabled(newValue: boolean) {
    this.enableLimit(newValue);
  }
  
  public enableLimit(flag: boolean): void {
    if (flag !== this.mEnableLimit) {
      this.mBodyA!.setAwake(true);
      this.mBodyB!.setAwake(true);
      this.mEnableLimit = flag;
      this.mImpulse.z = 0.0;
    }
  }
  
  public get lowerLimit(): number {
    return this.mLowerAngle;
  }
  
  public get upperLimit(): number {
    return this.mUpperAngle;
  }
  
  public setLimits(lower: number, upper: number): void {
    if (lower !== this.mLowerAngle || upper !== this.mUpperAngle) {
      this.mBodyA!.setAwake(true);
      this.mBodyB!.setAwake(true);
      this.mImpulse.z = 0.0;
      this.mLowerAngle = lower;
      this.mUpperAngle = upper;
    }
  }
  
  public get isMotorEnabled(): boolean {
    return this.mEnableMotor;
  }
  
  public enableMotor(flag: boolean): void {
    this.mBodyA!.setAwake(true);
    this.mBodyB!.setAwake(true);
    this.mEnableMotor = flag;
  }
  
  public setMotorSpeed(speed: number): void {
    this.mBodyA!.setAwake(true);
    this.mBodyB!.setAwake(true);
    this.mMotorSpeed = speed;
  }
  
  public get motorSpeed(): number {
    return this.mMotorSpeed;
  }
  public set motorSpeed(newValue: number) {
    this.setMotorSpeed(newValue);
  }
  
  public setMaxMotorTorque(torque: number): void {
    this.mBodyA!.setAwake(true);
    this.mBodyB!.setAwake(true);
    this.mMaxMotorTorque = torque;
  }
  public get maxMotorTorque(): number {
    return this.mMaxMotorTorque;
  }
  public set maxMotorTorque(newValue: number) {
    this.setMaxMotorTorque(newValue);
  }
  public getReactionForce(invdt: number): B2Vec2 {
    let mP = new B2Vec2(this.mImpulse.x, this.mImpulse.y);
    return multM(mP, invdt);
  }
  
  public getReactionTorque(invdt: number): number {
    return invdt * this.mImpulse.z;
  }
  
  public getMotorTorque(invdt: number): number {
    return invdt * this.mMotorImpulse;
  }
  
  constructor(def: B2RevoluteJointDef) {
    super(def);
    this.mLocalAnchorA = def.localAnchorA;
    this.mLocalAnchorB = def.localAnchorB;
    this.mReferenceAngle = def.referenceAngle;
    this.mImpulse = new B2Vec3(0.0, 0.0, 0.0);
    this.mMotorImpulse = 0.0;
    this.mLowerAngle = def.lowerAngle;
    this.mUpperAngle = def.upperAngle;
    this.mMaxMotorTorque = def.maxMotorTorque;
    this.mMotorSpeed = def.motorSpeed;
    this.mEnableLimit = def.enableLimit;
    this.mEnableMotor = def.enableMotor;
    this.mLimitState = B2LimitState.INACTIVELIMIT;
  }
  
  initVelocityConstraints(data: B2SolverData): void {
    this.mIndexA = this.mBodyA!.mIslandIndex;
    this.mIndexB = this.mBodyB!.mIslandIndex;
    this.mLocalCenterA = this.mBodyA!.mSweep.localCenter;
    this.mLocalCenterB = this.mBodyB!.mSweep.localCenter;
    this.mInvMassA = this.mBodyA!.mInvMass;
    this.mInvMassB = this.mBodyB!.mInvMass;
    this.mInvIA = this.mBodyA!.mInvI;
    this.mInvIB = this.mBodyB!.mInvI;
    let aA: number = data.positions.get(this.mIndexA).a;
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let aB: number = data.positions.get(this.mIndexB).a;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    this.mrA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    this.mrB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    let fixedRotation = iA + iB === 0.0;
    this.mMass.ex.x = mA + mB + this.mrA.y * this.mrA.y * iA + this.mrB.y * this.mrB.y * iB;
    this.mMass.ey.x = -this.mrA.y * this.mrA.x * iA - this.mrB.y * this.mrB.x * iB;
    this.mMass.ez.x = -this.mrA.y * iA - this.mrB.y * iB;
    this.mMass.ex.y = this.mMass.ey.x;
    this.mMass.ey.y = mA + mB + this.mrA.x * this.mrA.x * iA + this.mrB.x * this.mrB.x * iB;
    this.mMass.ez.y = this.mrA.x * iA + this.mrB.x * iB;
    this.mMass.ex.z = this.mMass.ez.x;
    this.mMass.ey.z = this.mMass.ez.y;
    this.mMass.ez.z = iA + iB;
    this.mMotorMass = iA + iB;
    if (this.mMotorMass > 0.0) {
      this.mMotorMass = 1.0 / this.mMotorMass;
    }
    if (this.mEnableMotor === false || fixedRotation) {
      this.mMotorImpulse = 0.0;
    }
    if (this.mEnableLimit && fixedRotation === false) {
      let jointAngle = aB - aA - this.mReferenceAngle;
      if (Math.abs(this.mUpperAngle - this.mLowerAngle) < float2 * b2AngularSlop) {
        this.mLimitState = B2LimitState.EQUALLIMITS;
      } else if (jointAngle <= this.mLowerAngle) {
        if (this.mLimitState !== B2LimitState.ATLOWERLIMIT) {
          this.mImpulse.z = 0.0;
        }
        this.mLimitState = B2LimitState.ATLOWERLIMIT;
      } else if (jointAngle >= this.mUpperAngle) {
        if (this.mLimitState !== B2LimitState.ATUPPERLIMIT) {
          this.mImpulse.z = 0.0;
        }
        this.mLimitState = B2LimitState.ATUPPERLIMIT;
      } else {
        this.mLimitState = B2LimitState.INACTIVELIMIT;
        this.mImpulse.z = 0.0;
      }
    } else {
      this.mLimitState = B2LimitState.INACTIVELIMIT;
    }
    if (data.step.warmStarting) {
      mulMEqual3(this.mImpulse, data.step.dtRatio);
      this.mMotorImpulse *= data.step.dtRatio;
      let mP = new B2Vec2(this.mImpulse.x, this.mImpulse.y);
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * (b2Cross(this.mrA, mP) + this.mMotorImpulse + this.mImpulse.z);
      addEqual(vB, multM(mP, mB));
      wB += iB * (b2Cross(this.mrB, mP) + this.mMotorImpulse + this.mImpulse.z);
    } else {
      this.mImpulse.setZero();
      this.mMotorImpulse = 0.0;
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solveVelocityConstraints(data: B2SolverData): void {
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    let fixedRotation = iA + iB === 0.0;
    if (this.mEnableMotor && this.mLimitState !== B2LimitState.EQUALLIMITS && fixedRotation === false) {
      let cdot = wB - wA - this.mMotorSpeed;
      let impulse = -this.mMotorMass * cdot;
      let oldImpulse = this.mMotorImpulse;
      let maxImpulse = data.step.dt * this.mMaxMotorTorque;
      this.mMotorImpulse = b2ClampF(this.mMotorImpulse + impulse, -maxImpulse, maxImpulse);
      impulse = this.mMotorImpulse - oldImpulse;
      wA -= iA * impulse;
      wB += iB * impulse;
    }
    
    if (this.mEnableLimit && this.mLimitState !== B2LimitState.INACTIVELIMIT && fixedRotation === false) {
      let cdot1 = subtract(subtract(add(vB, b2Cross21(wB, this.mrB)), vA), b2Cross21(wA, this.mrA));
      let cdot2 = wB - wA;
      let cdot = new B2Vec3(cdot1.x, cdot1.y, cdot2);
      let impulse = minus3(this.mMass.solve33(cdot));
      if (this.mLimitState === B2LimitState.EQUALLIMITS) {
        addEqual3(this.mImpulse, impulse);
      } else if (this.mLimitState === B2LimitState.ATLOWERLIMIT) {
        let newImpulse = this.mImpulse.z + impulse.z;
        if (newImpulse < 0.0) {
          let rhs = add(minus(cdot1), multM(new B2Vec2(this.mMass.ez.x, this.mMass.ez.y), this.mImpulse.z));
          let reduced = this.mMass.solve22(rhs);
          impulse.x = reduced.x;
          impulse.y = reduced.y;
          impulse.z = -this.mImpulse.z;
          this.mImpulse.x += reduced.x;
          this.mImpulse.y += reduced.y;
          this.mImpulse.z = 0.0;
        } else {
          addEqual3(this.mImpulse, impulse);
        }
      } else if (this.mLimitState === B2LimitState.ATUPPERLIMIT) {
        let newImpulse = this.mImpulse.z + impulse.z;
        if (newImpulse > 0.0) {
          let mass = new B2Vec2(this.mMass.ez.x, this.mMass.ez.y);
          let rhs = add(minus(cdot1), multM(mass, this.mImpulse.z));
          let reduced = this.mMass.solve22(rhs);
          impulse.x = reduced.x;
          impulse.y = reduced.y;
          impulse.z = -this.mImpulse.z;
          this.mImpulse.x += reduced.x;
          this.mImpulse.y += reduced.y;
          this.mImpulse.z = 0.0;
        } else {
          addEqual3(this.mImpulse, impulse);
        }
      }
      let mP = new B2Vec2(impulse.x, impulse.y);
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * (b2Cross(this.mrA, mP) + impulse.z);
      addEqual(vB, multM(mP, mB));
      wB += iB * (b2Cross(this.mrB, mP) + impulse.z);
    } else {
      let cdot = subtract(subtract(add(vB, b2Cross21(wB, this.mrB)), vA), b2Cross21(wA, this.mrA));
      let impulse = this.mMass.solve22(minus(cdot));
      this.mImpulse.x += impulse.x;
      this.mImpulse.y += impulse.y;
      subtractEqual(vA, multM(impulse, mA));
      wA -= iA * b2Cross(this.mrA, impulse);
      addEqual(vB, multM(impulse, mB));
      wB += iB * b2Cross(this.mrB, impulse);
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solvePositionConstraints(data: B2SolverData): boolean {
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    let angularError: number = 0.0;
    let positionError: number = 0.0;
    let fixedRotation = this.mInvIA + this.mInvIB === 0.0;
    if (this.mEnableLimit && this.mLimitState !== B2LimitState.INACTIVELIMIT && fixedRotation === false) {
      let angle = aB - aA - this.mReferenceAngle;
      let limitImpulse: number = 0.0;
      if (this.mLimitState === B2LimitState.EQUALLIMITS) {
        let mC = b2ClampF(angle - this.mLowerAngle, -b2MaxAngularCorrection, b2MaxAngularCorrection);
        limitImpulse = -this.mMotorMass * mC;
        angularError = Math.abs(mC);
      } else if (this.mLimitState === B2LimitState.ATLOWERLIMIT) {
        let mC = angle - this.mLowerAngle;
        angularError = -mC;
        mC = b2ClampF(mC + b2AngularSlop, -b2MaxAngularCorrection, 0.0);
        limitImpulse = -this.mMotorMass * mC;
      } else if (this.mLimitState === B2LimitState.ATUPPERLIMIT) {
        let mC = angle - this.mUpperAngle;
        angularError = mC;
        mC = b2ClampF(mC - b2AngularSlop, 0.0, b2MaxAngularCorrection);
        limitImpulse = -this.mMotorMass * mC;
      }
      aA -= this.mInvIA * limitImpulse;
      aB += this.mInvIB * limitImpulse;
    }
    qA.set(aA);
    qB.set(aB);
    let rA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    let rB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let cC = subtract(subtract(add(cB, rB), cA), rA);
    positionError = cC.length();
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    let mK = new B2Mat22();
    mK.ex.x = mA + mB + iA * rA.y * rA.y + iB * rB.y * rB.y;
    mK.ex.y = -iA * rA.x * rA.y - iB * rB.x * rB.y;
    mK.ey.x = mK.ex.y;
    mK.ey.y = mA + mB + iA * rA.x * rA.x + iB * rB.x * rB.x;
    let impulse = minus(mK.solve(cC));
    subtractEqual(cA, multM(impulse, mA));
    aA -= iA * b2Cross(rA, impulse);
    addEqual(cB, multM(impulse, mB));
    aB += iB * b2Cross(rB, impulse);
    data.positions.get(this.mIndexA).c = cA;
    data.positions.get(this.mIndexA).a = aA;
    data.positions.get(this.mIndexB).c = cB;
    data.positions.get(this.mIndexB).a = aB;
    return positionError <= b2LinearSlop && angularError <= b2AngularSlop;
  }
  mLocalAnchorA: B2Vec2;
  mLocalAnchorB: B2Vec2;
  mImpulse: B2Vec3;
  mMotorImpulse: number;
  mEnableMotor: boolean;
  mMaxMotorTorque: number;
  mMotorSpeed: number;
  mEnableLimit: boolean;
  mReferenceAngle: number;
  mLowerAngle: number;
  mUpperAngle: number;
  mIndexA: number = 0;
  mIndexB: number = 0;
  mrA = new B2Vec2();
  mrB = new B2Vec2();
  mLocalCenterA = new B2Vec2();
  mLocalCenterB = new B2Vec2();
  mInvMassA: number = 0.0;
  mInvMassB: number = 0.0;
  mInvIA: number = 0.0;
  mInvIB: number = 0.0;
  mMass = new B2Mat33();
  mMotorMass: number = 0.0;
  mLimitState = B2LimitState.INACTIVELIMIT;
}

export class B2PulleyJointDef extends B2JointDef {
  constructor() {
    super();
    this.groundAnchorA = new B2Vec2(-1.0, 1.0);
    this.groundAnchorB = new B2Vec2(1.0, 1.0);
    this.localAnchorA = new B2Vec2(-1.0, 0.0);
    this.localAnchorB = new B2Vec2(1.0, 0.0);
    this.lengthA = 0.0;
    this.lengthB = 0.0;
    this.ratio = 1.0;
    this.type = B2JointType.PULLEYJOINT;
    this.collideConnected = true;
  }
  
  public initialize(bodyA: B2Body, bodyB: B2Body, groundAnchorA: B2Vec2, groundAnchorB: B2Vec2, anchorA: B2Vec2, anchorB: B2Vec2, ratio: number): void {
    this.bodyA = bodyA;
    this.bodyB = bodyB;
    this.groundAnchorA = groundAnchorA;
    this.groundAnchorB = groundAnchorB;
    this.localAnchorA = this.bodyA.getLocalPoint(anchorA);
    this.localAnchorB = this.bodyB.getLocalPoint(anchorB);
    let dA = subtract(anchorA, groundAnchorA);
    this.lengthA = dA.length();
    let dB = subtract(anchorB, groundAnchorB);
    this.lengthB = dB.length();
    this.ratio = ratio;
  }
  
  groundAnchorA: B2Vec2;
  groundAnchorB: B2Vec2;
  localAnchorA: B2Vec2;
  localAnchorB: B2Vec2;
  lengthA: number;
  lengthB: number;
  ratio: number;
}

export class B2PulleyJoint extends B2Joint {
  mGroundAnchorA: B2Vec2;
  mGroundAnchorB: B2Vec2;
  mLengthA: number;
  mLengthB: number;
  mLocalAnchorA: B2Vec2;
  mLocalAnchorB: B2Vec2;
  mConstant: number;
  mRatio: number;
  mImpulse: number;
  mIndexA: number = 0;
  mIndexB: number = 0;
  muA = new B2Vec2();
  muB = new B2Vec2();
  mrA = new B2Vec2();
  mrB = new B2Vec2();
  mLocalCenterA = new B2Vec2();
  mLocalCenterB = new B2Vec2();
  mInvMassA: number = 0.0;
  mInvMassB: number = 0.0;
  mInvIA: number = 0.0;
  mInvIB: number = 0.0;
  mMass: number = 0.0;
  public get anchorA(): B2Vec2 {
    return this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
  }
  public get anchorB(): B2Vec2 {
    return this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
  }
  public getReactionForce(invdt: number): B2Vec2 {
    let mP = multM(this.muB, this.mImpulse);
    return multM(mP, invdt);
  }
  public getReactionTorque(invdt: number): number {
    return 0.0;
  }
  public get groundAnchorA(): B2Vec2 {
    return this.mGroundAnchorA;
  }
  public get groundAnchorB(): B2Vec2 {
    return this.mGroundAnchorB;
  }
  public get lengthA(): number {
    return this.mLengthA;
  }
  public get lengthB(): number {
    return this.mLengthB;
  }
  public get ratio(): number {
    return this.mRatio;
  }
  public get currentLengthA(): number {
    let p = this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
    let s = this.mGroundAnchorA;
    let d = subtract(p, s);
    return d.length();
  }
  public get currentLengthB(): number {
    let p = this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
    let s = this.mGroundAnchorB;
    let d = subtract(p, s);
    return d.length();
  }
  public shiftOrigin(newOrigin: B2Vec2): void {
    subtractEqual(this.mGroundAnchorA, newOrigin);
    subtractEqual(this.mGroundAnchorB, newOrigin);
  }
  constructor(def: B2PulleyJointDef) {
    super(def);
    this.mGroundAnchorA = def.groundAnchorA;
    this.mGroundAnchorB = def.groundAnchorB;
    this.mLocalAnchorA = def.localAnchorA;
    this.mLocalAnchorB = def.localAnchorB;
    this.mLengthA = def.lengthA;
    this.mLengthB = def.lengthB;
    this.mRatio = def.ratio;
    this.mConstant = def.lengthA + this.mRatio * def.lengthB;
    this.mImpulse = 0.0;
  }
  
  initVelocityConstraints(data: B2SolverData): void {
    this.mIndexA = this.mBodyA!.mIslandIndex;
    this.mIndexB = this.mBodyB!.mIslandIndex;
    this.mLocalCenterA = this.mBodyA!.mSweep.localCenter;
    this.mLocalCenterB = this.mBodyB!.mSweep.localCenter;
    this.mInvMassA = this.mBodyA!.mInvMass;
    this.mInvMassB = this.mBodyB!.mInvMass;
    this.mInvIA = this.mBodyA!.mInvI;
    this.mInvIB = this.mBodyB!.mInvI;
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    this.mrA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    this.mrB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    this.muA = subtract(add(cA, this.mrA), this.mGroundAnchorA);
    this.muB = subtract(add(cB, this.mrB), this.mGroundAnchorB);
    let lengthA = this.muA.length();
    let lengthB = this.muB.length();
    if (lengthA > float10 * b2LinearSlop) {
      mulEqual(this.muA, 1.0 / lengthA);
    } else {
      this.muA.setZero();
    }
    if (lengthB > float10 * b2LinearSlop) {
      mulEqual(this.muB, 1.0 / lengthB);
    } else {
      this.muB.setZero();
    }
    let ruA = b2Cross(this.mrA, this.muA);
    let ruB = b2Cross(this.mrB, this.muB);
    let mA = this.mInvMassA + this.mInvIA * ruA * ruA;
    let mB = this.mInvMassB + this.mInvIB * ruB * ruB;
    this.mMass = mA + this.mRatio * this.mRatio * mB;
    this.mMass = this.mMass > 0.0 ? 1.0 / this.mMass : this.mMass;
    if (data.step.warmStarting) {
      this.mImpulse *= data.step.dtRatio;
      let mPA = multM(this.muA, -this.mImpulse);
      let mPB = multM(this.muB, -this.mRatio * this.mImpulse);
      addEqual(vA, multM(mPA, this.mInvMassA));
      wA += this.mInvIA * b2Cross(this.mrA, mPA);
      addEqual(vB, multM(mPB, this.mInvMassB));
      wB += this.mInvIB * b2Cross(this.mrB, mPB);
    } else {
      this.mImpulse = 0.0;
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solveVelocityConstraints(data: B2SolverData): void {
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let vpA = add(vA, b2Cross21(wA, this.mrA));
    let vpB = add(vB, b2Cross21(wB, this.mrB));
    let cdot = -b2Dot22(this.muA, vpA) - this.mRatio * b2Dot22(this.muB, vpB);
    let impulse = -this.mMass * cdot;
    this.mImpulse += impulse;
    let mPA = multM(this.muA, -impulse);
    let mPB = multM(this.muB, -this.mRatio * impulse);
    addEqual(vA, multM(mPA, this.mInvMassA));
    wA += this.mInvIA * b2Cross(this.mrA, mPA);
    addEqual(vB, multM(mPB, this.mInvMassB));
    wB += this.mInvIB * b2Cross(this.mrB, mPB);
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solvePositionConstraints(data: B2SolverData): boolean {
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    let rA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    let rB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let uA = subtract(add(cA, rA), this.mGroundAnchorA);
    let uB = subtract(add(cB, rB), this.mGroundAnchorB);
    let lengthA = uA.length();
    let lengthB = uB.length();
    if (lengthA > float10 * b2LinearSlop) {
      mulEqual(uA, 1.0 / lengthA);
    } else {
      uA.setZero();
    }
    if (lengthB > float10 * b2LinearSlop) {
      mulEqual(uB, 1.0 / lengthB);
    } else {
      uB.setZero();
    }
    let ruA = b2Cross(rA, uA);
    let ruB = b2Cross(rB, uB);
    let mA = this.mInvMassA + this.mInvIA * ruA * ruA;
    let mB = this.mInvMassB + this.mInvIB * ruB * ruB;
    let mass = mA + this.mRatio * this.mRatio * mB;
    if (mass > 0.0) {
      mass = 1.0 / mass;
    }
    let mC = this.mConstant - lengthA - this.mRatio * lengthB;
    let linearError = Math.abs(mC);
    let impulse = -mass * mC;
    let mPA = multM(uA, impulse);
    let mPB = multM(uB, -this.mRatio * impulse);
    addEqual(cA, multM(mPA, this.mInvMassA));
    aA += this.mInvIA * b2Cross(rA, mPA);
    addEqual(cB, multM(mPB, this.mInvMassB));
    aB += this.mInvIB * b2Cross(rB, mPB);
    data.positions.get(this.mIndexA).c = cA;
    data.positions.get(this.mIndexA).a = aA;
    data.positions.get(this.mIndexB).c = cB;
    data.positions.get(this.mIndexB).a = aB;
    return linearError < b2LinearSlop;
  }
}

export class B2PrismaticJointDef extends B2JointDef {
  constructor() {
    super();
    this.localAnchorA = new B2Vec2();
    this.localAnchorB = new B2Vec2();
    this.localAxisA = new B2Vec2(1.0, 0.0);
    this.referenceAngle = 0.0;
    this.enableLimit = false;
    this.lowerTranslation = 0.0;
    this.upperTranslation = 0.0;
    this.enableMotor = false;
    this.maxMotorForce = 0.0;
    this.motorSpeed = 0.0;
    this.type = B2JointType.PRISMATICJOINT;
  }
  
  public initialize(bA: B2Body, bB: B2Body, anchor: B2Vec2, axis: B2Vec2): void {
    this.bodyA = bA;
    this.bodyB = bB;
    this.localAnchorA = this.bodyA.getLocalPoint(anchor);
    this.localAnchorB = this.bodyB.getLocalPoint(anchor);
    this.localAxisA = this.bodyA.getLocalVector(axis);
    this.referenceAngle = this.bodyB.angle - this.bodyA.angle;
  }
  localAnchorA: B2Vec2;
  localAnchorB: B2Vec2;
  localAxisA: B2Vec2;
  referenceAngle: number;
  enableLimit: boolean;
  lowerTranslation: number;
  upperTranslation: number;
  enableMotor: boolean;
  maxMotorForce: number;
  motorSpeed: number;
}

export class B2PrismaticJoint extends B2Joint {
  public get anchorA(): B2Vec2 {
    return this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
  }
  
  public get anchorB(): B2Vec2 {
    return this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
  }
  
  public getReactionForce(invdt: number): B2Vec2 {
    return multM(add(multM(this.mPerp, this.mImpulse.x), multM(this.mAxis, this.mMotorImpulse + this.mImpulse.z)), invdt);
  }
  
  public getReactionTorque(invdt: number): number {
    return invdt * this.mImpulse.y;
  }
  
  public get localAnchorA(): B2Vec2 {
    return this.mLocalAnchorA;
  }
  
  public get localAnchorB(): B2Vec2 {
    return this.mLocalAnchorB;
  }
  
  public get localAxisA(): B2Vec2 {
    return this.mLocalXAxisA;
  }
  
  public get referenceAngle(): number {
    return this.mReferenceAngle;
  }
  
  public jointTranslation(): number {
    let pA = this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
    let pB = this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
    let d = subtract(pB, pA);
    let axis = this.mBodyA!.getWorldVector(this.mLocalXAxisA);
    let translation = b2Dot22(d, axis);
    return translation;
  }
  
  public jointSpeed(): number {
    let bA = this.mBodyA;
    let bB = this.mBodyB;
    let rA = b2MulR2(bA!.mXF.q, subtract(this.mLocalAnchorA, bA!.mSweep.localCenter));
    let rB = b2MulR2(bB!.mXF.q, subtract(this.mLocalAnchorB, bB!.mSweep.localCenter));
    let p1 = add(bA!.mSweep.c, rA);
    let p2 = add(bB!.mSweep.c, rB);
    let d = subtract(p2, p1);
    let axis = b2MulR2(bA!.mXF.q, this.mLocalXAxisA);
    let vA = bA!.mLinearVelocity;
    let vB = bB!.mLinearVelocity;
    let wA = bA!.mAngularVelocity;
    let wB = bB!.mAngularVelocity;
    let sub = subtract(subtract(add(vB, b2Cross21(wB, rB)), vA), b2Cross21(wA, rA));
    let speed = b2Dot22(d, b2Cross21(wA, axis)) + b2Dot22(axis, sub);
    return speed;
  }
  
  get isLimitEnabled(): boolean {
    return this.mEnableLimit;
  }
  setisLimitEnabled(newValue: boolean): void {
    this.enableLimit(newValue);
  }
  
  public enableLimit(flag: boolean): void {
    if (flag !== this.mEnableLimit) {
      this.mBodyA!.setAwake(true);
      this.mBodyB!.setAwake(true);
      this.mEnableLimit = flag;
      this.mImpulse.z = 0.0;
    }
  }
  
  public get lowerLimit(): number {
    return this.mLowerTranslation;
  }
  
  public get upperLimit(): number {
    return this.mUpperTranslation;
  }
  
  public setLimits(lower: number, upper: number): void {
    if (lower !== this.mLowerTranslation || upper !== this.mUpperTranslation) {
      this.mBodyA!.setAwake(true);
      this.mBodyB!.setAwake(true);
      this.mLowerTranslation = lower;
      this.mUpperTranslation = upper;
      this.mImpulse.z = 0.0;
    }
  }
  
  get isMotorEnabled(): boolean {
    return this.mEnableMotor;
  }
  setisMotorEnabled(newValue: boolean): void {
    this.enableMotor(newValue);
  }
  
  public enableMotor(flag: boolean): void {
    this.mBodyA!.setAwake(true);
    this.mBodyB!.setAwake(true);
    this.mEnableMotor = flag;
  }
  
  setMotorSpeed(speed: number): void {
    this.mBodyA!.setAwake(true);
    this.mBodyB!.setAwake(true);
    this.mMotorSpeed = speed;
  }
  get motorSpeed(): number {
    return this.mMotorSpeed;
  }
  set motorSpeed(newValue: number) {
    this.setMotorSpeed(newValue);
  }
  
  setMaxMotorForce(force: number): void {
    this.mBodyA!.setAwake(true);
    this.mBodyB!.setAwake(true);
    this.mMaxMotorForce = force;
  }
  
  get maxMotorForce(): number {
    return this.mMaxMotorForce;
  }
  set maxMotorForce(newValue: number) {
    this.setMaxMotorForce(newValue);
  }
  
  getMotorForce(invdt: number): number {
    return invdt * this.mMotorImpulse;
  }
  
  constructor(def: B2PrismaticJointDef) {
    super(def);
    this.mLocalAnchorA = def.localAnchorA;
    this.mLocalAnchorB = def.localAnchorB;
    this.mLocalXAxisA = def.localAxisA;
    this.mLocalXAxisA.normalize();
    this.mLocalYAxisA = b2Cross21(1.0, this.mLocalXAxisA);
    this.mReferenceAngle = def.referenceAngle;
    this.mImpulse = new B2Vec3(0.0, 0.0, 0.0);
    this.mMotorMass = 0.0;
    this.mMotorImpulse = 0.0;
    this.mLowerTranslation = def.lowerTranslation;
    this.mUpperTranslation = def.upperTranslation;
    this.mMaxMotorForce = def.maxMotorForce;
    this.mMotorSpeed = def.motorSpeed;
    this.mEnableLimit = def.enableLimit;
    this.mEnableMotor = def.enableMotor;
    this.mLimitState = B2LimitState.INACTIVELIMIT;
    this.mAxis = new B2Vec2(0.0, 0.0);
    this.mPerp = new B2Vec2(0.0, 0.0);
  }
  
  initVelocityConstraints(data: B2SolverData): void {
    this.mIndexA = this.mBodyA!.mIslandIndex;
    this.mIndexB = this.mBodyB!.mIslandIndex;
    this.mLocalCenterA = this.mBodyA!.mSweep.localCenter;
    this.mLocalCenterB = this.mBodyB!.mSweep.localCenter;
    this.mInvMassA = this.mBodyA!.mInvMass;
    this.mInvMassB = this.mBodyB!.mInvMass;
    this.mInvIA = this.mBodyA!.mInvI;
    this.mInvIB = this.mBodyB!.mInvI;
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    let rA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    let rB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let d = subtract(add(subtract(cB, cA), rB), rA);
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    this.mAxis = b2MulR2(qA, this.mLocalXAxisA);
    this.ma1 = b2Cross(add(d, rA), this.mAxis);
    this.ma2 = b2Cross(rB, this.mAxis);
    this.mMotorMass = mA + mB + iA * this.ma1 * this.ma1 + iB * this.ma2 * this.ma2;
    if (this.mMotorMass > 0.0) {
      this.mMotorMass = 1.0 / this.mMotorMass;
    }
    this.mPerp = b2MulR2(qA, this.mLocalYAxisA);
    this.ms1 = b2Cross(add(d, rA), this.mPerp);
    this.ms2 = b2Cross(rB, this.mPerp);
    let k11 = mA + mB + iA * this.ms1 * this.ms1 + iB * this.ms2 * this.ms2;
    let k12 = iA * this.ms1 + iB * this.ms2;
    let k13 = iA * this.ms1 * this.ma1 + iB * this.ms2 * this.ma2;
    let k22 = iA + iB;
    if (k22 === 0.0) {
      k22 = 1.0;
    }
    let k23 = iA * this.ma1 + iB * this.ma2;
    let k33 = mA + mB + iA * this.ma1 * this.ma1 + iB * this.ma2 * this.ma2;
    this.matK.ex.set(k11, k12, k13);
    this.matK.ey.set(k12, k22, k23);
    this.matK.ez.set(k13, k23, k33);
    if (this.mEnableLimit) {
      let jointTranslation = b2Dot22(this.mAxis, d);
      if (Math.abs(this.mUpperTranslation - this.mLowerTranslation) < float2 * b2LinearSlop) {
        this.mLimitState = B2LimitState.EQUALLIMITS;
      } else if (jointTranslation <= this.mLowerTranslation) {
        if (this.mLimitState !== B2LimitState.ATLOWERLIMIT) {
          this.mLimitState = B2LimitState.ATLOWERLIMIT;
          this.mImpulse.z = 0.0;
        }
      } else if (jointTranslation >= this.mUpperTranslation) {
        if (this.mLimitState !== B2LimitState.ATUPPERLIMIT) {
          this.mLimitState = B2LimitState.ATUPPERLIMIT;
          this.mImpulse.z = 0.0;
        }
      } else {
        this.mLimitState = B2LimitState.INACTIVELIMIT;
        this.mImpulse.z = 0.0;
      }
    } else {
      this.mLimitState = B2LimitState.INACTIVELIMIT;
      this.mImpulse.z = 0.0;
    }
    if (this.mEnableMotor === false) {
      this.mMotorImpulse = 0.0;
    }
    if (data.step.warmStarting) {
      mulMEqual3(this.mImpulse, data.step.dtRatio);
      this.mMotorImpulse *= data.step.dtRatio;
      let mP = add(multM(this.mPerp, this.mImpulse.x), multM(this.mAxis, this.mMotorImpulse + this.mImpulse.z));
      let mLA = this.mImpulse.x * this.ms1 + this.mImpulse.y + (this.mMotorImpulse + this.mImpulse.z) * this.ma1;
      let mLB = this.mImpulse.x * this.ms2 + this.mImpulse.y + (this.mMotorImpulse + this.mImpulse.z) * this.ma2;
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * mLA;
      addEqual(vB, multM(mP, mB));
      wB += iB * mLB;
    } else {
      this.mImpulse.setZero();
      this.mMotorImpulse = 0.0;
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solveVelocityConstraints(data: B2SolverData): void {
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    if (this.mEnableMotor && this.mLimitState !== B2LimitState.EQUALLIMITS) {
      let cdot = b2Dot22(this.mAxis, subtract(vB, vA)) + this.ma2 * wB - this.ma1 * wA;
      let impulse = this.mMotorMass * (this.mMotorSpeed - cdot);
      let oldImpulse = this.mMotorImpulse;
      let maxImpulse = data.step.dt * this.mMaxMotorForce;
      this.mMotorImpulse = b2ClampF(this.mMotorImpulse + impulse, -maxImpulse, maxImpulse);
      impulse = this.mMotorImpulse - oldImpulse;
      let mP = multM(this.mAxis, impulse);
      let lA = impulse * this.ma1;
      let lB = impulse * this.ma2;
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * lA;
      addEqual(vB, multM(mP, mB));
      wB += iB * lB;
    }
    let cdot1 = new B2Vec2();
    cdot1.x = b2Dot22(this.mPerp, subtract(vB, vA)) + this.ms2 * wB - this.ms1 * wA;
    cdot1.y = wB - wA;
    if (this.mEnableLimit && this.mLimitState !== B2LimitState.INACTIVELIMIT) {
      let cdot2 = b2Dot22(this.mAxis, subtract(vB, vA)) + this.ma2 * wB - this.ma1 * wA;
      let cdot = new B2Vec3(cdot1.x, cdot1.y, cdot2);
      let f1 = this.mImpulse;
      let df = this.matK.solve33(minus3(cdot));
      addEqual3(this.mImpulse, df);
      if (this.mLimitState === B2LimitState.ATLOWERLIMIT) {
        this.mImpulse.z = Math.max(this.mImpulse.z, 0.0);
      } else if (this.mLimitState === B2LimitState.ATUPPERLIMIT) {
        this.mImpulse.z = Math.min(this.mImpulse.z, 0.0);
      }
      let mkk = new B2Vec2(this.matK.ez.x, this.matK.ez.y);
      let b = subtract(minus(cdot1), multM(mkk, this.mImpulse.z - f1.z));
      let ffx = new B2Vec2(f1.x, f1.y);
      let f2r = add(this.matK.solve22(b), ffx);
      this.mImpulse.x = f2r.x;
      this.mImpulse.y = f2r.y;
      df = subtract3(this.mImpulse, f1);
      let p = add(multM(this.mPerp, df.x), multM(this.mAxis, df.z));
      let lA = df.x * this.ms1 + df.y + df.z * this.ma1;
      let lB = df.x * this.ms2 + df.y + df.z * this.ma2;
      subtractEqual(vA, multM(p, mA));
      wA -= iA * lA;
      addEqual(vB, multM(p, mB));
      wB += iB * lB;
    } else {
      let df = this.matK.solve22(minus(cdot1));
      this.mImpulse.x += df.x;
      this.mImpulse.y += df.y;
      let mP = multM(this.mPerp, df.x);
      let mLA = df.x * this.ms1 + df.y;
      let mLB = df.x * this.ms2 + df.y;
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * mLA;
      addEqual(vB, multM(mP, mB));
      wB += iB * mLB;
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solvePositionConstraints(data: B2SolverData): boolean {
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    let rA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    let rB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let d = subtract(subtract(add(cB, rB), cA), rA);
    let axis = b2MulR2(qA, this.mLocalXAxisA);
    let a1 = b2Cross(add(d, rA), axis);
    let a2 = b2Cross(rB, axis);
    let perp = b2MulR2(qA, this.mLocalYAxisA);
    let s1 = b2Cross(add(d, rA), perp);
    let s2 = b2Cross(rB, perp);
    let impulse = new B2Vec3();
    let c1 = new B2Vec2();
    c1.x = b2Dot22(perp, d);
    c1.y = aB - aA - this.mReferenceAngle;
    let linearError = Math.abs(c1.x);
    let angularError = Math.abs(c1.y);
    let active = false;
    let c2: number = 0.0;
    if (this.mEnableLimit) {
      let translation = b2Dot22(axis, d);
      if (Math.abs(this.mUpperTranslation - this.mLowerTranslation) < float2 * b2LinearSlop) {
        c2 = b2ClampF(translation, -b2MaxLinearCorrection, b2MaxLinearCorrection);
        linearError = Math.max(linearError, Math.abs(translation));
        active = true;
      } else if (translation <= this.mLowerTranslation) {
        c2 = b2ClampF(translation - this.mLowerTranslation + b2LinearSlop, -b2MaxLinearCorrection, 0.0);
        linearError = Math.max(linearError, this.mLowerTranslation - translation);
        active = true;
      } else if (translation >= this.mUpperTranslation) {
        c2 = b2ClampF(translation - this.mUpperTranslation - b2LinearSlop, 0.0, b2MaxLinearCorrection);
        linearError = Math.max(linearError, translation - this.mUpperTranslation);
        active = true;
      }
    }
    if (active) {
      let k11 = mA + mB + iA * s1 * s1 + iB * s2 * s2;
      let k12 = iA * s1 + iB * s2;
      let k13 = iA * s1 * a1 + iB * s2 * a2;
      let k22 = iA + iB;
      if (k22 === 0.0) {
        k22 = 1.0;
      }
      let k23 = iA * a1 + iB * a2;
      let k33 = mA + mB + iA * a1 * a1 + iB * a2 * a2;
      let mK = new B2Mat33();
      mK.ex.set(k11, k12, k13);
      mK.ey.set(k12, k22, k23);
      mK.ez.set(k13, k23, k33);
      let cC = new B2Vec3();
      cC.x = c1.x;
      cC.y = c1.y;
      cC.z = c2;
      impulse = mK.solve33(minus3(cC));
    } else {
      let k11 = mA + mB + iA * s1 * s1 + iB * s2 * s2;
      let k12 = iA * s1 + iB * s2;
      let k22 = iA + iB;
      if (k22 === 0.0) {
        k22 = 1.0;
      }
      let mK = new B2Mat22();
      mK.ex.set(k11, k12);
      mK.ey.set(k12, k22);
      let impulse1 = mK.solve(minus(c1));
      impulse.x = impulse1.x;
      impulse.y = impulse1.y;
      impulse.z = 0.0;
    }
    let p = add(multM(perp, impulse.x), multM(axis, impulse.z));
    let lA = impulse.x * s1 + impulse.y + impulse.z * a1;
    let lB = impulse.x * s2 + impulse.y + impulse.z * a2;
    subtractEqual(cA, multM(p, mA));
    aA -= iA * lA;
    addEqual(cB, multM(p, mB));
    aB += iB * lB;
    data.positions.get(this.mIndexA).c = cA;
    data.positions.get(this.mIndexA).a = aA;
    data.positions.get(this.mIndexB).c = cB;
    data.positions.get(this.mIndexB).a = aB;
    return linearError <= b2LinearSlop && angularError <= b2AngularSlop;
  }
  
  mLocalAnchorA: B2Vec2;
  mLocalAnchorB: B2Vec2;
  mLocalXAxisA: B2Vec2;
  mLocalYAxisA: B2Vec2;
  mReferenceAngle: number;
  mImpulse: B2Vec3;
  mMotorImpulse: number;
  mLowerTranslation: number;
  mUpperTranslation: number;
  mMaxMotorForce: number;
  mMotorSpeed: number;
  mEnableLimit: boolean;
  mEnableMotor: boolean;
  mLimitState: B2LimitState;
  mIndexA: number = 0;
  mIndexB: number = 0;
  mLocalCenterA = new B2Vec2();
  mLocalCenterB = new B2Vec2();
  mInvMassA: number = 0.0;
  mInvMassB: number = 0.0;
  mInvIA: number = 0.0;
  mInvIB: number = 0.0;
  mAxis = new B2Vec2();
  mPerp = new B2Vec2();
  ms1: number = 0.0;
  ms2: number = 0.0;
  ma1: number = 0.0;
  ma2: number = 0.0;
  matK = new B2Mat33();
  mMotorMass: number = 0.0;
}

export class B2MouseJointDef extends B2JointDef {
  public constructor() {
    super();
    this.target = new B2Vec2();
    this.maxForce = 0.0;
    this.frequencyHz = 5.0;
    this.dampingRatio = 0.7;
    this.type = B2JointType.MOUSEJOINT;
  }
  public target: B2Vec2;
  public maxForce: number;
  public frequencyHz: number;
  public dampingRatio: number;
}

export class B2MouseJoint extends B2Joint {
  public get anchorA(): B2Vec2 {
    return this.mTargetA;
  }
  public get anchorB(): B2Vec2 {
    return this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
  }
  public getReactionForce(invdt: number): B2Vec2 {
    return multM(this.mImpulse, invdt);
  }
  public getReactionTorque(invdt: number): number {
    return invdt * 0.0;
  }
  public setTarget(target: B2Vec2): void {
    if (this.mBodyB!.isAwake === false) {
      this.mBodyB!.setAwake(true);
    }
    this.mTargetA = target;
  }
  get target(): B2Vec2 {
    return this.mTargetA;
  }
  set target(newValue: B2Vec2) {
    this.setTarget(newValue);
  }
  public setMaxForce(force: number): void {
    this.mMaxForce = force;
  }
  get maxForce(): number {
    return this.mMaxForce;
  }
  set force(newValue: number) {
    this.setMaxForce(newValue);
  }
  public setFrequency(hz: number): void {
    this.mFrequencyHz = hz;
  }
  get hz(): number {
    return this.mFrequencyHz;
  }
  set hz(newValue: number) {
    this.setFrequency(newValue);
  }
  public setDampingRatio(ratio: number): void {
    this.mDampingRatio = ratio;
  }
  get dampingRatio(): number {
    return this.mDampingRatio;
  }
  set dampingRatio(newValue: number) {
    this.setDampingRatio(newValue);
  }
  public shiftOrigin(newOrigin: B2Vec2): void {
    subtractEqual(this.mTargetA, newOrigin);
  }
  
  constructor(def: B2MouseJointDef) {
    super(def);
    this.mTargetA = def.target;
    this.mMaxForce = def.maxForce;
    this.mImpulse = new B2Vec2(0.0, 0.0);
    this.mFrequencyHz = def.frequencyHz;
    this.mDampingRatio = def.dampingRatio;
    this.mBeta = 0.0;
    this.mGamma = 0.0;
    this.mLocalAnchorB = b2MulTT2(this.mBodyB!.transform, this.mTargetA);
  }
  
  initVelocityConstraints(data: B2SolverData): void {
    let multiFactor: number = 0.98;
    this.mIndexB = this.mBodyB!.mIslandIndex;
    this.mLocalCenterB = this.mBodyB!.mSweep.localCenter;
    this.mInvMassB = this.mBodyB!.mInvMass;
    this.mInvIB = this.mBodyB!.mInvI;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let qB = new B2Rot(aB);
    let mass = this.mBodyB!.mass;
    let omega = float2 * b2PI * this.mFrequencyHz;
    let d = float2 * mass * this.mDampingRatio * omega;
    let k = mass * (omega * omega);
    let h = data.step.dt;
    this.mGamma = h * (d + h * k);
    if (this.mGamma !== 0.0) {
      this.mGamma = 1.0 / this.mGamma;
    }
    this.mBeta = h * k * this.mGamma;
    this.mrB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let mK = new B2Mat22();
    mK.ex.x = this.mInvMassB + this.mInvIB * this.mrB.y * this.mrB.y + this.mGamma;
    mK.ex.y = -this.mInvIB * this.mrB.x * this.mrB.y;
    mK.ey.x = mK.ex.y;
    mK.ey.y = this.mInvMassB + this.mInvIB * this.mrB.x * this.mrB.x + this.mGamma;
    this.mMass = mK.getInverse();
    this.mC = subtract(add(cB, this.mrB), this.mTargetA);
    mulEqual(this.mC, this.mBeta);
    wB *= multiFactor;
    if (data.step.warmStarting) {
      mulEqual(this.mImpulse, data.step.dtRatio);
      addEqual(vB, multM(this.mImpulse, this.mInvMassB));
      wB += this.mInvIB * b2Cross(this.mrB, this.mImpulse);
    } else {
      this.mImpulse.setZero();
    }
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solveVelocityConstraints(data: B2SolverData): void {
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let cdot = add(vB, b2Cross21(wB, this.mrB));
    let impulse = b2Mul22(this.mMass, minus(add(add(cdot, this.mC), multM(this.mImpulse, this.mGamma))));
    let oldImpulse = this.mImpulse;
    addEqual(this.mImpulse, impulse);
    let maxImpulse = data.step.dt * this.mMaxForce;
    if (this.mImpulse.lengthSquared() > maxImpulse * maxImpulse) {
      mulEqual(this.mImpulse, maxImpulse / this.mImpulse.length());
    }
    impulse = subtract(this.mImpulse, oldImpulse);
    addEqual(vB, multM(impulse, this.mInvMassB));
    wB += this.mInvIB * b2Cross(this.mrB, impulse);
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  solvePositionConstraints(data: B2SolverData): boolean {
    return true;
  }
  mLocalAnchorB: B2Vec2;
  mTargetA: B2Vec2;
  mFrequencyHz: number;
  mDampingRatio: number;
  mBeta: number;
  mImpulse: B2Vec2;
  mMaxForce: number;
  mGamma: number;
  mIndexA: number = 0;
  mIndexB: number = 0;
  mrB = new B2Vec2();
  mLocalCenterB = new B2Vec2();
  mInvMassB: number = 0.0;
  mInvIB: number = 0.0;
  mMass = new B2Mat22();
  mC = new B2Vec2();
}

export class B2GearJointDef extends B2JointDef {
  public constructor() {
    super();
    this.joint1 = null;
    this.joint2 = null;
    this.ratio = 1.0;
    this.type = B2JointType.GEARJOINT;
  }
  joint1: B2Joint | null = null;
  joint2: B2Joint | null = null;
  ratio: number = 1.0;
}

export class B2GearJoint extends B2Joint {
  public get anchorA(): B2Vec2 {
    return this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
  }
  public get anchorB(): B2Vec2 {
    return this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
  }
  
  public getReactionForce(invdt: number): B2Vec2 {
    let mP = multM(this.mJvAC, this.mImpulse);
    return multM(mP, invdt);
  }
  
  public getReactionTorque(invdt: number): number {
    let mL = this.mImpulse * this.mJwA;
    return invdt * mL;
  }
  
  public get joint1(): B2Joint | null {
    return this.mJoint1;
  }
  
  public get joint2(): B2Joint | null {
    return this.mJoint2;
  }
  
  public setRatio(ratio: number): void {
    this.mRatio = ratio;
  }
  
  public get ratio(): number {
    return this.mRatio;
  }
  public set ratio(newValue: number) {
    this.setRatio(newValue);
  }
  
  constructor(def: B2GearJointDef) {
    super(def);
    this.mJoint1 = def.joint1;
    this.mJoint2 = def.joint2;
    this.mTypeA = this.mJoint1!.typeJoint;
    this.mTypeB = this.mJoint2!.typeJoint;
    let coordinateA: number;
    let coordinateB: number;
    this.mBodyC = this.mJoint1!.bodyA!;
    this.mBodyA = this.mJoint1!.bodyB!;
    let xfA = this.mBodyA!.mXF;
    let aA = this.mBodyA!.mSweep.a;
    let xfC = this.mBodyC.mXF;
    let aC = this.mBodyC.mSweep.a;
    if (this.mTypeA === B2JointType.REVOLUTEJOINT) {
      let revolute = def.joint1 as B2RevoluteJoint;
      this.mLocalAnchorC = revolute.mLocalAnchorA;
      this.mLocalAnchorA = revolute.mLocalAnchorB;
      this.mReferenceAngleA = revolute.mReferenceAngle;
      this.mLocalAxisC.setZero();
      coordinateA = aA - aC - this.mReferenceAngleA;
    } else {
      let prismatic = def.joint1 as B2PrismaticJoint;
      this.mLocalAnchorC = prismatic.mLocalAnchorA;
      this.mLocalAnchorA = prismatic.mLocalAnchorB;
      this.mReferenceAngleA = prismatic.mReferenceAngle;
      this.mLocalAxisC = prismatic.mLocalXAxisA;
      let pC = this.mLocalAnchorC;
      let pA = b2MulTR2(xfC.q, add(b2MulR2(xfA.q, this.mLocalAnchorA), subtract(xfA.p, xfC.p)));
      coordinateA = b2Dot22(subtract(pA, pC), this.mLocalAxisC);
    }
    this.mBodyD = this.mJoint2!.bodyA!;
    this.mBodyB = this.mJoint2!.bodyB;
    let xfB = this.mBodyB!.mXF;
    let aB = this.mBodyB!.mSweep.a;
    let xfD = this.mBodyD.mXF;
    let aD = this.mBodyD.mSweep.a;
    if (this.mTypeB === B2JointType.REVOLUTEJOINT) {
      let revolute = def.joint2 as B2RevoluteJoint;
      this.mLocalAnchorD = revolute.mLocalAnchorA;
      this.mLocalAnchorB = revolute.mLocalAnchorB;
      this.mReferenceAngleB = revolute.mReferenceAngle;
      this.mLocalAxisD.setZero();
      coordinateB = aB - aD - this.mReferenceAngleB;
    } else {
      let prismatic = def.joint2 as B2PrismaticJoint;
      this.mLocalAnchorD = prismatic.mLocalAnchorA;
      this.mLocalAnchorB = prismatic.mLocalAnchorB;
      this.mReferenceAngleB = prismatic.mReferenceAngle;
      this.mLocalAxisD = prismatic.mLocalXAxisA;
      let pD = this.mLocalAnchorD;
      let pB = b2MulTR2(xfD.q, add(b2MulR2(xfB.q, this.mLocalAnchorB), subtract(xfB.p, xfD.p)));
      coordinateB = b2Dot22(subtract(pB, pD), this.mLocalAxisD);
    }
    this.mRatio = def.ratio;
    this.mConstant = coordinateA + this.mRatio * coordinateB;
    this.mImpulse = 0.0;
  }
  
  initVelocityConstraints(data: B2SolverData): void {
    this.mIndexA = this.mBodyA!.mIslandIndex;
    this.mIndexB = this.mBodyB!.mIslandIndex;
    this.mIndexC = this.mBodyC.mIslandIndex;
    this.mIndexD = this.mBodyD.mIslandIndex;
    this.mLcA = this.mBodyA!.mSweep.localCenter;
    this.mLcB = this.mBodyB!.mSweep.localCenter;
    this.mLcC = this.mBodyC.mSweep.localCenter;
    this.mLcD = this.mBodyD.mSweep.localCenter;
    this.mmA = this.mBodyA!.mInvMass;
    this.mmB = this.mBodyB!.mInvMass;
    this.mmC = this.mBodyC.mInvMass;
    this.mmD = this.mBodyD.mInvMass;
    this.miA = this.mBodyA!.mInvI;
    this.miB = this.mBodyB!.mInvI;
    this.miC = this.mBodyC.mInvI;
    this.miD = this.mBodyD.mInvI;
    let aA: number = data.positions.get(this.mIndexA).a;
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let aB: number = data.positions.get(this.mIndexB).a;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let aC: number = data.positions.get(this.mIndexC).a;
    let vC: B2Vec2 = data.velocities.get(this.mIndexC).v;
    let wC: number = data.velocities.get(this.mIndexC).w;
    let aD: number = data.positions.get(this.mIndexD).a;
    let vD: B2Vec2 = data.velocities.get(this.mIndexD).v;
    let wD: number = data.velocities.get(this.mIndexD).w;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    let qC = new B2Rot(aC);
    let qD = new B2Rot(aD);
    this.mMass = 0.0;
    if (this.mTypeA === B2JointType.REVOLUTEJOINT) {
      this.mJvAC.setZero();
      this.mJwA = 1.0;
      this.mJwC = 1.0;
      this.mMass += this.miA + this.miC;
    } else {
      let u = b2MulR2(qC, this.mLocalAxisC);
      let rC = b2MulR2(qC, subtract(this.mLocalAnchorC, this.mLcC));
      let rA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLcA));
      this.mJvAC = u;
      this.mJwC = b2Cross(rC, u);
      this.mJwA = b2Cross(rA, u);
      this.mMass += this.mmC + this.mmA + this.miC * this.mJwC * this.mJwC + this.miA * this.mJwA * this.mJwA;
    }
    if (this.mTypeB === B2JointType.REVOLUTEJOINT) {
      this.mJvBD.setZero();
      this.mJwB = this.mRatio;
      this.mJwD = this.mRatio;
      this.mMass += this.mRatio * this.mRatio * (this.miB + this.miD);
    } else {
      let u = b2MulR2(qD, this.mLocalAxisD);
      let rD = b2MulR2(qD, subtract(this.mLocalAnchorD, this.mLcD));
      let rB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLcB));
      this.mJvBD = multM(u, this.mRatio);
      this.mJwD = this.mRatio * b2Cross(rD, u);
      this.mJwB = this.mRatio * b2Cross(rB, u);
      let mass1 = this.mRatio * this.mRatio * (this.mmD + this.mmB);
      let mass2 = this.miD * this.mJwD * this.mJwD + this.miB * this.mJwB * this.mJwB;
      let mass = mass1 + mass2;
      this.mMass += mass;
    }
    this.mMass = this.mMass > 0.0 ? 1.0 / this.mMass : 0.0;
    if (data.step.warmStarting) {
      addEqual(vA, multM(this.mJvAC, this.mmA * this.mImpulse));
      wA += this.miA * this.mImpulse * this.mJwA;
      addEqual(vB, multM(this.mJvBD, this.mmB * this.mImpulse));
      wB += this.miB * this.mImpulse * this.mJwB;
      subtractEqual(vC, multM(this.mJvAC, this.mmC * this.mImpulse));
      wC -= this.miC * this.mImpulse * this.mJwC;
      subtractEqual(vD, multM(this.mJvBD, this.mmD * this.mImpulse));
      wD -= this.miD * this.mImpulse * this.mJwD;
    } else {
      this.mImpulse = 0.0;
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
    data.velocities.get(this.mIndexC).v = vC;
    data.velocities.get(this.mIndexC).w = wC;
    data.velocities.get(this.mIndexD).v = vD;
    data.velocities.get(this.mIndexD).w = wD;
  }
  
  solveVelocityConstraints(data: B2SolverData): void {
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let vC: B2Vec2 = data.velocities.get(this.mIndexC).v;
    let wC: number = data.velocities.get(this.mIndexC).w;
    let vD: B2Vec2 = data.velocities.get(this.mIndexD).v;
    let wD: number = data.velocities.get(this.mIndexD).w;
    let cdot = b2Dot22(this.mJvAC, subtract(vA, vC)) + b2Dot22(this.mJvBD, subtract(vB, vD));
    cdot += this.mJwA * wA - this.mJwC * wC + (this.mJwB * wB - this.mJwD * wD);
    let impulse = -this.mMass * cdot;
    this.mImpulse += impulse;
    addEqual(vA, multM(this.mJvAC, this.mmA * impulse));
    wA += this.miA * impulse * this.mJwA;
    addEqual(vB, multM(this.mJvBD, this.mmB * impulse));
    wB += this.miB * impulse * this.mJwB;
    subtractEqual(vC, multM(this.mJvAC, this.mmC * impulse));
    wC -= this.miC * impulse * this.mJwC;
    subtractEqual(vD, multM(this.mJvBD, this.mmD * impulse));
    wD -= this.miD * impulse * this.mJwD;
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
    data.velocities.get(this.mIndexC).v = vC;
    data.velocities.get(this.mIndexC).w = wC;
    data.velocities.get(this.mIndexD).v = vD;
    data.velocities.get(this.mIndexD).w = wD;
  }
  
  solvePositionConstraints(data: B2SolverData): boolean {
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let cC: B2Vec2 = data.positions.get(this.mIndexC).c;
    let aC: number = data.positions.get(this.mIndexC).a;
    let cD: B2Vec2 = data.positions.get(this.mIndexD).c;
    let aD: number = data.positions.get(this.mIndexD).a;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    let qC = new B2Rot(aC);
    let qD = new B2Rot(aD);
    let linearError: number = 0.0;
    let coordinateA: number = 0;
    let coordinateB: number = 0;
    let mJvAC = new B2Vec2();
    let mJvBD = new B2Vec2();
    let mJwA: number = 0;
    let mJwB: number = 0;
    let mJwC: number = 0;
    let mJwD: number = 0;
    let mass: number = 0.0;
    if (this.mTypeA === B2JointType.REVOLUTEJOINT) {
      mJvAC.setZero();
      mJwA = 1.0;
      mJwC = 1.0;
      mass += this.miA + this.miC;
      coordinateA = aA - aC - this.mReferenceAngleA;
    } else {
      let u = b2MulR2(qC, this.mLocalAxisC);
      let rC = b2MulR2(qC, subtract(this.mLocalAnchorC, this.mLcC));
      let rA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLcA));
      mJvAC = u;
      mJwC = b2Cross(rC, u);
      mJwA = b2Cross(rA, u);
      mass += this.mmC + this.mmA + this.miC * mJwC * mJwC + this.miA * mJwA * mJwA;
      let pC = subtract(this.mLocalAnchorC, this.mLcC);
      let pA = b2MulTR2(qC, add(rA, subtract(cA, cC)));
      coordinateA = b2Dot22(subtract(pA, pC), this.mLocalAxisC);
    }
    if (this.mTypeB === B2JointType.REVOLUTEJOINT) {
      mJvBD.setZero();
      mJwB = this.mRatio;
      mJwD = this.mRatio;
      mass += this.mRatio * this.mRatio * (this.miB + this.miD);
      coordinateB = aB - aD - this.mReferenceAngleB;
    } else {
      let u = b2MulR2(qD, this.mLocalAxisD);
      let rD = b2MulR2(qD, subtract(this.mLocalAnchorD, this.mLcD));
      let rB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLcB));
      mJvBD = multM(u, this.mRatio);
      mJwD = this.mRatio * b2Cross(rD, u);
      mJwB = this.mRatio * b2Cross(rB, u);
      mass += this.mRatio * this.mRatio * (this.mmD + this.mmB) + this.miD * mJwD * mJwD + this.miB * mJwB * mJwB;
      let pD = subtract(this.mLocalAnchorD, this.mLcD);
      let pB = b2MulTR2(qD, add(rB, subtract(cB, cD)));
      coordinateB = b2Dot22(subtract(pB, pD), this.mLocalAxisD);
    }
    let mC = coordinateA + this.mRatio * coordinateB - this.mConstant;
    let impulse: number = 0.0;
    if (mass > 0.0) {
      impulse = -mC / mass;
    }
    addEqual(cA, multM(mJvAC, this.mmA * impulse));
    aA += this.miA * impulse * mJwA;
    addEqual(cB, multM(mJvBD, this.mmB * impulse));
    aB += this.miB * impulse * mJwB;
    subtractEqual(cC, multM(mJvAC, this.mmC * impulse));
    aC -= this.miC * impulse * mJwC;
    subtractEqual(cD, multM(mJvBD, this.mmD * impulse));
    aD -= this.miD * impulse * mJwD;
    data.positions.get(this.mIndexA).c = cA;
    data.positions.get(this.mIndexA).a = aA;
    data.positions.get(this.mIndexB).c = cB;
    data.positions.get(this.mIndexB).a = aB;
    data.positions.get(this.mIndexC).c = cC;
    data.positions.get(this.mIndexC).a = aC;
    data.positions.get(this.mIndexD).c = cD;
    data.positions.get(this.mIndexD).a = aD;
    return linearError < b2LinearSlop;
  }
  mJoint1: B2Joint | null;
  mJoint2: B2Joint | null;
  mTypeA: B2JointType;
  mTypeB: B2JointType;
  mBodyC: B2Body;
  mBodyD: B2Body;
  mLocalAnchorA = new B2Vec2();
  mLocalAnchorB = new B2Vec2();
  mLocalAnchorC = new B2Vec2();
  mLocalAnchorD = new B2Vec2();
  mLocalAxisC = new B2Vec2();
  mLocalAxisD = new B2Vec2();
  mReferenceAngleA: number = 0.0;
  mReferenceAngleB: number = 0.0;
  mConstant: number = 0.0;
  mRatio: number = 0.0;
  mImpulse: number = 0.0;
  mIndexA: number = 0;
  mIndexB: number = 0;
  mIndexC: number = 0;
  mIndexD: number = 0;
  mLcA = new B2Vec2();
  mLcB = new B2Vec2();
  mLcC = new B2Vec2();
  mLcD = new B2Vec2();
  mmA: number = 0.0;
  mmB: number = 0.0;
  mmC: number = 0.0;
  mmD: number = 0.0;
  miA: number = 0.0;
  miB: number = 0.0;
  miC: number = 0.0;
  miD: number = 0.0;
  mJvAC = new B2Vec2();
  mJvBD = new B2Vec2();
  mJwA: number = 0.0;
  mJwB: number = 0.0;
  mJwC: number = 0.0;
  mJwD: number = 0.0;
  mMass: number = 0.0;
}

export class B2FrictionJointDef extends B2JointDef {
  constructor() {
    super();
    this.localAnchorA = new B2Vec2();
    this.localAnchorB = new B2Vec2();
    this.maxForce = 0.0;
    this.maxTorque = 0.0;
    this.type = B2JointType.FRICTIONJOINT;
  }

  public initialize(bA: B2Body, bB: B2Body, anchor: B2Vec2): void {
    this.bodyA = bA;
    this.bodyB = bB;
    this.localAnchorA = this.bodyA.getLocalPoint(anchor);
    this.localAnchorB = this.bodyB.getLocalPoint(anchor);
  }

  public localAnchorA: B2Vec2;
  public localAnchorB: B2Vec2;
  public maxForce: number;
  public maxTorque: number;
}

export class B2FrictionJoint extends B2Joint {
  mLocalAnchorA: B2Vec2;
  mLocalAnchorB: B2Vec2;
  mLinearImpulse: B2Vec2;
  mAngularImpulse: number;
  mMaxForce: number;
  mMaxTorque: number;
  mIndexA: number = 0;
  mIndexB: number = 0;
  mrA = new B2Vec2();
  mrB = new B2Vec2();
  mLocalCenterA = new B2Vec2();
  mLocalCenterB = new B2Vec2();
  mInvMassA: number = 0.0;
  mInvMassB: number = 0.0;
  mInvIA: number = 0.0;
  mInvIB: number = 0.0;
  mLinearMass = new B2Mat22();
  mAngularMass: number = 0.0;
  public get anchorA(): B2Vec2 {
    return this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
  }

  public get anchorB(): B2Vec2 {
    return this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
  }

  public getReactionForce(invdt: number): B2Vec2 {
    return multM(this.mLinearImpulse, invdt);
  }

  public getReactionTorque(invdt: number): number {
    return invdt * this.mAngularImpulse;
  }

  public get localAnchorA(): B2Vec2 {
    return this.mLocalAnchorA;
  }

  public get localAnchorB(): B2Vec2 {
    return this.mLocalAnchorB;
  }

  public setMaxForce(force: number): void {
    this.mMaxForce = force;
  }

  public get maxForce(): number {
    return this.mMaxForce;
  }

  public setMaxTorque(torque: number): void {
    this.mMaxTorque = torque;
  }

  public get maxTorque(): number {
    return this.mMaxTorque;
  }

  constructor(def: B2FrictionJointDef) {
    super(def);
    this.mLocalAnchorA = def.localAnchorA;
    this.mLocalAnchorB = def.localAnchorB;
    this.mLinearImpulse = new B2Vec2(0.0, 0.0);
    this.mAngularImpulse = 0.0;
    this.mMaxForce = def.maxForce;
    this.mMaxTorque = def.maxTorque;
  }

  initVelocityConstraints(data: B2SolverData): void {
    this.mIndexA = this.mBodyA!.mIslandIndex;
    this.mIndexB = this.mBodyB!.mIslandIndex;
    this.mLocalCenterA = this.mBodyA!.mSweep.localCenter;
    this.mLocalCenterB = this.mBodyB!.mSweep.localCenter;
    this.mInvMassA = this.mBodyA!.mInvMass;
    this.mInvMassB = this.mBodyB!.mInvMass;
    this.mInvIA = this.mBodyA!.mInvI;
    this.mInvIB = this.mBodyB!.mInvI;
    let aA: number = data.positions.get(this.mIndexA).a;
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let aB: number = data.positions.get(this.mIndexB).a;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    this.mrA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    this.mrB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    let mK = new B2Mat22();
    mK.ex.x = mA + mB + iA * this.mrA.y * this.mrA.y + iB * this.mrB.y * this.mrB.y;
    mK.ex.y = -iA * this.mrA.x * this.mrA.y - iB * this.mrB.x * this.mrB.y;
    mK.ey.x = mK.ex.y;
    mK.ey.y = mA + mB + iA * this.mrA.x * this.mrA.x + iB * this.mrB.x * this.mrB.x;
    this.mLinearMass = mK.getInverse();
    this.mAngularMass = iA + iB;
    if (this.mAngularMass > 0.0) {
      this.mAngularMass = 1.0 / this.mAngularMass;
    }
    if (data.step.warmStarting) {
      mulEqual(this.mLinearImpulse, data.step.dtRatio);
      this.mAngularImpulse *= data.step.dtRatio;
      let mP = new B2Vec2(this.mLinearImpulse.x, this.mLinearImpulse.y);
      subtractEqual(vA, multM(mP, mA));
      wA -= iA * (b2Cross(this.mrA, mP) + this.mAngularImpulse);
      addEqual(vB, multM(mP, mB));
      wB += iB * (b2Cross(this.mrB, mP) + this.mAngularImpulse);
    } else {
      this.mLinearImpulse.setZero();
      this.mAngularImpulse = 0.0;
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }

  solveVelocityConstraints(data: B2SolverData): void {
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let mA = this.mInvMassA;
    let mB = this.mInvMassB;
    let iA = this.mInvIA;
    let iB = this.mInvIB;
    let h = data.step.dt;
    let cdot = wB - wA;
    let impulse = -this.mAngularMass * cdot;
    let oldImpulse = this.mAngularImpulse;
    let maxImpulse = h * this.mMaxTorque;
    this.mAngularImpulse = b2ClampF(this.mAngularImpulse + impulse, -maxImpulse, maxImpulse);
    impulse = this.mAngularImpulse - oldImpulse;
    wA -= iA * impulse;
    wB += iB * impulse;
    let locCdot = subtract(subtract(add(vB, b2Cross21(wB, this.mrB)), vA), b2Cross21(wA, this.mrA));
    let locImpulse = minus(b2Mul22(this.mLinearMass, locCdot));
    let locOldImpulse = this.mLinearImpulse;
    addEqual(this.mLinearImpulse, locImpulse);
    let locMaxImpulse = h * this.mMaxForce;
    if (this.mLinearImpulse.lengthSquared() > locMaxImpulse * locMaxImpulse) {
      this.mLinearImpulse.normalize();
      mulEqual(this.mLinearImpulse, locMaxImpulse);
    }
    locImpulse = subtract(this.mLinearImpulse, locOldImpulse);
    subtractEqual(vA, multM(locImpulse, mA));
    wA -= iA * b2Cross(this.mrA, locImpulse);
    addEqual(vB, multM(locImpulse, mB));
    wB += iB * b2Cross(this.mrB, locImpulse);
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }

  solvePositionConstraints(data: B2SolverData): boolean {
    return true;
  }
}

export class B2DistanceJointDef extends B2JointDef {
  constructor(bodyA?: B2Body, bodyB?: B2Body, anchorA?: B2Vec2, anchorB?: B2Vec2) {
    super();
    if (bodyA !== null && bodyB !== null && anchorA !== null && anchorB !== null) {
      this.initialize(bodyA!, bodyB!, anchorA!, anchorB!);
    } else {
      this.localAnchorA = new B2Vec2(0.0, 0.0);
      this.localAnchorB = new B2Vec2(0.0, 0.0);
      this.length = 1.0;
      this.frequencyHz = 0.0;
      this.dampingRatio = 0.0;
      this.type = B2JointType.DISTANCEJOINT;
    }
  }
  
  initialize(bA: B2Body, bB: B2Body, anchorA: B2Vec2, anchorB: B2Vec2): void {
    this.bodyA = bA;
    this.bodyB = bB;
    this.localAnchorA = this.bodyA.getLocalPoint(anchorA);
    this.localAnchorB = this.bodyB.getLocalPoint(anchorB);
    let d = subtract(anchorB, anchorA);
    this.length = d.length();
  }
  
  public localAnchorA: B2Vec2 = new B2Vec2();
  public localAnchorB: B2Vec2 = new B2Vec2();
  public length: number = 1.0;
  public frequencyHz: number = 0.0;
  public dampingRatio: number = 0.0;
}

export class B2DistanceJoint extends B2Joint {
  mFrequencyHz: number;
  mDampingRatio: number;
  mBias: number;
  mLocalAnchorA: B2Vec2;
  mLocalAnchorB: B2Vec2;
  mGamma: number;
  mImpulse: number;
  mLength: number;
  mIndexA: number = 0;
  mIndexB: number = 0;
  mu = new B2Vec2();
  mrA = new B2Vec2();
  mrB = new B2Vec2();
  mLocalCenterA = new B2Vec2();
  mLocalCenterB = new B2Vec2();
  mInvMassA: number = 0.0;
  mInvMassB: number = 0.0;
  mInvIA: number = 0.0;
  mInvIB: number = 0.0;
  mMass: number = 0.0;
  public get anchorA(): B2Vec2 {
    return this.mBodyA!.getWorldPoint(this.mLocalAnchorA);
  }
  
  public get anchorB(): B2Vec2 {
    return this.mBodyB!.getWorldPoint(this.mLocalAnchorB);
  }
  
  public getReactionForce(invdt: number): B2Vec2 {
    let mF = multM(this.mu, invdt * this.mImpulse);
    return mF;
  }
  
  public getReactionTorque(invdt: number): number {
    return 0.0;
  }
  
  public get localAnchorA(): B2Vec2 {
    return this.mLocalAnchorA;
  }
  
  public get localAnchorB(): B2Vec2 {
    return this.mLocalAnchorB;
  }
  
  setLength(length: number): void {
    this.mLength = length;
  }
  
  public get length(): number {
    return this.mLength;
  }
  
  setFrequency(hz: number): void {
    this.mFrequencyHz = hz;
  }
  
  public get frequency(): number {
    return this.mFrequencyHz;
  }
  
  setDampingRatio(ratio: number): void {
    this.mDampingRatio = ratio;
  }
  
  public get dampingRatio(): number {
    return this.mDampingRatio;
  }
  
  constructor(def: B2DistanceJointDef) {
    super(def);
    this.mLocalAnchorA = def.localAnchorA;
    this.mLocalAnchorB = def.localAnchorB;
    this.mLength = def.length;
    this.mFrequencyHz = def.frequencyHz;
    this.mDampingRatio = def.dampingRatio;
    this.mImpulse = 0.0;
    this.mGamma = 0.0;
    this.mBias = 0.0;
  }
  
  public initVelocityConstraints(data: B2SolverData): void {
    this.mIndexA = this.mBodyA!.mIslandIndex;
    this.mIndexB = this.mBodyB!.mIslandIndex;
    this.mLocalCenterA = this.mBodyA!.mSweep.localCenter;
    this.mLocalCenterB = this.mBodyB!.mSweep.localCenter;
    this.mInvMassA = this.mBodyA!.mInvMass;
    this.mInvMassB = this.mBodyB!.mInvMass;
    this.mInvIA = this.mBodyA!.mInvI;
    this.mInvIB = this.mBodyB!.mInvI;
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    this.mrA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    this.mrB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    this.mu = subtract(subtract(add(cB, this.mrB), cA), this.mrA);
    let length = this.mu.length();
    if (length > b2LinearSlop) {
      mulEqual(this.mu, 1.0 / length);
    } else {
      this.mu.set(0.0, 0.0);
    }
    let crAu = b2Cross(this.mrA, this.mu);
    let crBu = b2Cross(this.mrB, this.mu);
    let invMass = this.mInvMassA + this.mInvIA * crAu * crAu + this.mInvMassB + this.mInvIB * crBu * crBu;
    this.mMass = invMass !== 0.0 ? 1.0 / invMass : 0.0;
    if (this.mFrequencyHz > 0.0) {
      let mC = length - this.mLength;
      let omega = float2 * b2PI * this.mFrequencyHz;
      let d = float2 * this.mMass * this.mDampingRatio * omega;
      let k = this.mMass * omega * omega;
      let h = data.step.dt;
      this.mGamma = h * (d + h * k);
      this.mGamma = this.mGamma !== 0.0 ? 1.0 / this.mGamma : 0.0;
      this.mBias = mC * h * k * this.mGamma;
      invMass += this.mGamma;
      this.mMass = invMass !== 0.0 ? 1.0 / invMass : 0.0;
    } else {
      this.mGamma = 0.0;
      this.mBias = 0.0;
    }
    if (data.step.warmStarting) {
      this.mImpulse *= data.step.dtRatio;
      let mP = multM(this.mu, this.mImpulse);
      subtractEqual(vA, multM(mP, this.mInvMassA));
      wA -= this.mInvIA * b2Cross(this.mrA, mP);
      addEqual(vB, multM(mP, this.mInvMassB));
      wB += this.mInvIB * b2Cross(this.mrB, mP);
    } else {
      this.mImpulse = 0.0;
    }
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  public solveVelocityConstraints(data: B2SolverData): void {
    let vA: B2Vec2 = data.velocities.get(this.mIndexA).v;
    let wA: number = data.velocities.get(this.mIndexA).w;
    let vB: B2Vec2 = data.velocities.get(this.mIndexB).v;
    let wB: number = data.velocities.get(this.mIndexB).w;
    let vpA = add(vA, b2Cross21(wA, this.mrA));
    let vpB = add(vB, b2Cross21(wB, this.mrB));
    let cdot = b2Dot22(this.mu, subtract(vpB, vpA));
    let impulse = -this.mMass * (cdot + this.mBias + this.mGamma * this.mImpulse);
    this.mImpulse += impulse;
    let mP = multM(this.mu, impulse);
    subtractEqual(vA, multM(mP, this.mInvMassA));
    wA -= this.mInvIA * b2Cross(this.mrA, mP);
    addEqual(vB, multM(mP, this.mInvMassB));
    wB += this.mInvIB * b2Cross(this.mrB, mP);
    data.velocities.get(this.mIndexA).v = vA;
    data.velocities.get(this.mIndexA).w = wA;
    data.velocities.get(this.mIndexB).v = vB;
    data.velocities.get(this.mIndexB).w = wB;
  }
  
  public solvePositionConstraints(data: B2SolverData): boolean {
    if (this.mFrequencyHz > 0.0) {
      return true;
    }
    let cA: B2Vec2 = data.positions.get(this.mIndexA).c;
    let aA: number = data.positions.get(this.mIndexA).a;
    let cB: B2Vec2 = data.positions.get(this.mIndexB).c;
    let aB: number = data.positions.get(this.mIndexB).a;
    let qA = new B2Rot(aA);
    let qB = new B2Rot(aB);
    let rA = b2MulR2(qA, subtract(this.mLocalAnchorA, this.mLocalCenterA));
    let rB = b2MulR2(qB, subtract(this.mLocalAnchorB, this.mLocalCenterB));
    let u = subtract(subtract(add(cB, rB), cA), rA);
    let length = u.normalize();
    let mC = length - this.mLength;
    mC = b2ClampF(mC, -b2MaxLinearCorrection, b2MaxLinearCorrection);
    let impulse = -this.mMass * mC;
    let mP = multM(u, impulse);
    subtractEqual(cA, multM(mP, this.mInvMassA));
    aA -= this.mInvIA * b2Cross(rA, mP);
    addEqual(cB, multM(mP, this.mInvMassB));
    aB += this.mInvIB * b2Cross(rB, mP);
    data.positions.get(this.mIndexA).c = cA;
    data.positions.get(this.mIndexA).a = aA;
    data.positions.get(this.mIndexB).c = cB;
    data.positions.get(this.mIndexB).a = aB;
    return Math.abs(mC) < b2LinearSlop;
  }
}
export { B2JointType, B2LimitState };
