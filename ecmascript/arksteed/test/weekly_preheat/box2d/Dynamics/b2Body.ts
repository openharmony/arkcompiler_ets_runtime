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
  b2MulTT2,
  b2MulTR2,
  B2Mat22,
  B2Transform,
  b2Vec2Zero,
  subtract,
  multM,
  add,
  b2Cross,
  b2Cross21,
  b2MulT2,
  b2MulR2,
  B2Sweep,
  b2Dot22,
  addEqual,
  mulEqual
} from '../Common/b2Math';
import { B2FixtureDef, B2Fixture } from './b2Fixture';
import { B2Shape } from '../Collision/Shapes/b2Shape';
import { B2MassData } from '../Collision/Shapes/b2Shape';
import { B2World } from './b2World';
import { FlagsW } from './b2World';
import { B2ContactEdge } from './Contacts/b2Contact';
import { B2JointEdge } from './Joints/b2Joint';
import { hex1, hex10, hex2, hex20, hex4, hex40, hex8 } from '../Common/b2Settings';
enum B2BodyType {
  STATICBODY = 0,
  KINEMATICBODY,
  DYNAMICBODY
}
export class B2BodyDef {
  type = B2BodyType.STATICBODY;
  position = new B2Vec2();
  angle: number = 0.0;
  linearVelocity = new B2Vec2();
  angularVelocity: number = 0.0;
  linearDamping: number = 0.0;
  angularDamping: number = 0.0;
  allowSleep = true;
  awake = true;
  fixedRotation = false;
  bullet = false;
  active = true;
  userData?: object | null = null;
  gravityScale: number = 1.0;
  constructor() {}
}
export class B2Body {
  mType: B2BodyType;
  mFlags: number = 0;
  mIslandIndex = 0;
  mXF: B2Transform;
  mSweep: B2Sweep;
  mLinearVelocity: B2Vec2;
  mAngularVelocity: number;
  mForce: B2Vec2;
  mTorque: number;
  mWorld: B2World;
  mPrev: B2Body | null = null;
  mNext: B2Body | null = null;
  mFixtureList: B2Fixture | null = null;
  mFixtureCount: number = 0;
  mJointList: B2JointEdge | null = null;
  mContactList: B2ContactEdge | null = null;
  mMass: number;
  mInvMass: number;
  mI: number;
  mInvI: number;
  mLinearDamping: number;
  mAngularDamping: number;
  mGravityScale: number;
  mSleepTime: number;
  mUserData: object | null;
  createFixture(def: B2FixtureDef): B2Fixture {
    if (this.mWorld.isLocked === true) {
      throw new Error('world is locked');
    }
    let fixture = new B2Fixture(this, def);
    if ((this.mFlags & Flags.activeFlag) !== 0) {
      let broadPhase = this.mWorld.mContactManager.mBroadPhase;
      fixture.createProxies(broadPhase, this.mXF);
    }
    fixture.mNext = this.mFixtureList;
    this.mFixtureList = fixture;
    this.mFixtureCount += 1;
    fixture.mBody = this;
    if (fixture.mDensity > 0.0) {
      this.resetMassData();
    }
    this.mWorld.mFlags |= FlagsW.newFixture;
    return fixture;
  }
  
  createFixture2(shape: B2Shape, density: number): B2Fixture {
    let def = new B2FixtureDef();
    def.shape = shape;
    def.density = density;
    return this.createFixture(def);
  }
  
  destroyFixture(fixture: B2Fixture): void {
    if (this.mWorld.isLocked === true) {
      return;
    }
    let prev: B2Fixture | null = null;
    let f = this.mFixtureList;
    while (f !== null) {
      if (f === fixture) {
        if (prev !== null) {
          prev.mNext = f!.mNext;
        } else {
          this.mFixtureList = f.mNext;
        }
      }
      prev = f;
      f = f.mNext;
    }
    
    let edge: B2ContactEdge | null = this.mContactList;
    while (edge !== null) {
      let c = edge!.contact;
      edge = edge!.next;
      let fixtureA = c.fixtureA;
      let fixtureB = c.fixtureB;
      if (fixture === fixtureA || fixture === fixtureB) {
        this.mWorld.mContactManager.destroy(c);
      }
    }
    if ((this.mFlags & Flags.activeFlag) !== 0) {
      let broadPhase = this.mWorld.mContactManager.mBroadPhase;
      fixture.destroyProxies(broadPhase);
    }
    fixture.destroy();
    fixture.mNext = null;
    this.mFixtureCount -= 1;
    this.resetMassData();
  }
  
  setTransform(position: B2Vec2, angle: number): void {
    if (this.mWorld.isLocked === true) {
      return;
    }
    this.mXF.q.set(angle);
    this.mXF.p = position;
    this.mSweep.c = b2MulT2(this.mXF, this.mSweep.localCenter);
    this.mSweep.a = angle;
    this.mSweep.c0 = this.mSweep.c;
    this.mSweep.a0 = angle;
    let broadPhase = this.mWorld.mContactManager.mBroadPhase;
    let f: B2Fixture | null = this.mFixtureList;
    while (f !== null) {
      f.synchronize(broadPhase, this.mXF, this.mXF);
      f = f!.mNext;
    }
  }
  
  get transform(): B2Transform {
    return this.mXF;
  }
  
  get position(): B2Vec2 {
    return this.mXF.p;
  }
  
  get angle(): number {
    return this.mSweep.a;
  }
  
  get worldCenter(): B2Vec2 {
    return this.mSweep.c;
  }
  
  get localCenter(): B2Vec2 {
    return this.mSweep.localCenter;
  }
  
  setLinearVelocity(v: B2Vec2): void {
    if (this.mType === B2BodyType.STATICBODY) {
      return;
    }
    if (b2Dot22(v, v) > 0.0) {
      this.setAwake(true);
    }
    this.mLinearVelocity = v;
  }
  
  get linearVelocity(): B2Vec2 {
    return this.mLinearVelocity;
  }
  
  set linearVelocity(newValue) {
    this.setLinearVelocity(newValue);
  }
  
  setAngularVelocity(omega: number): void {
    if (this.mType === B2BodyType.STATICBODY) {
      return;
    }
    if (omega * omega > 0.0) {
      this.setAwake(true);
    }
    this.mAngularVelocity = omega;
  }
  
  get angularVelocity(): number {
    return this.mAngularVelocity;
  }
  
  set angularVelocity(newValue) {
    this.setAngularVelocity(newValue);
  }
  
  applyForce(force: B2Vec2, point: B2Vec2, wake: boolean): void {
    if (this.mType !== B2BodyType.DYNAMICBODY) {
      return;
    }
    if (wake && (this.mFlags & Flags.awakeFlag) === 0) {
      this.setAwake(true);
    }
    if ((this.mFlags & Flags.awakeFlag) !== 0) {
      addEqual(this.mForce, force);
      this.mTorque += b2Cross(subtract(point, this.mSweep.c), force);
    }
  }
  
  applyForceToCenter(force: B2Vec2, wake: boolean): void {
    if (this.mType !== B2BodyType.DYNAMICBODY) {
      return;
    }
    if (wake && (this.mFlags & Flags.awakeFlag) === 0) {
      this.setAwake(true);
    }
    if ((this.mFlags & Flags.awakeFlag) !== 0) {
      addEqual(this.mForce, force);
    }
  }
  
  applyTorque(torque: number, wake: boolean): void {
    if (this.mType !== B2BodyType.DYNAMICBODY) {
      return;
    }
    if (wake && (this.mFlags & Flags.awakeFlag) === 0) {
      this.setAwake(true);
    }
    if ((this.mFlags & Flags.awakeFlag) !== 0) {
      this.mTorque += torque;
    }
  }
  
  applyLinearImpulse(impulse: B2Vec2, point: B2Vec2, wake: boolean): void {
    if (this.mType !== B2BodyType.DYNAMICBODY) {
      return;
    }
    if (wake && (this.mFlags & Flags.awakeFlag) === 0) {
      this.setAwake(true);
    }
    if ((this.mFlags & Flags.awakeFlag) !== 0) {
      addEqual(this.mLinearVelocity, multM(impulse, this.mInvMass));
      this.mAngularVelocity += this.mInvI * b2Cross(subtract(point, this.mSweep.c), impulse);
    }
  }
  
  applyAngularImpulse(impulse: number, wake: boolean): void {
    if (this.mType !== B2BodyType.DYNAMICBODY) {
      return;
    }
    if (wake && (this.mFlags & Flags.awakeFlag) === 0) {
      this.setAwake(true);
    }
    if ((this.mFlags & Flags.awakeFlag) !== 0) {
      this.mAngularVelocity += this.mInvI * impulse;
    }
  }
  get mass(): number {
    return this.mMass;
  }
  get inertia(): number {
    return this.mI + this.mMass * b2Dot22(this.mSweep.localCenter, this.mSweep.localCenter);
  }
  get massData(): B2MassData {
    let data = new B2MassData();
    data.mass = this.mMass;
    data.aI = this.mI + this.mMass * b2Dot22(this.mSweep.localCenter, this.mSweep.localCenter);
    data.center = this.mSweep.localCenter;
    return data;
  }
  
  set massData(newValue) {
    this.setMassData(newValue);
  }
  
  setMassData(massData: B2MassData): void {
    if (this.mWorld.isLocked === true) {
      return;
    }
    if (this.mType !== B2BodyType.DYNAMICBODY) {
      return;
    }
    this.mInvMass = 0.0;
    this.mI = 0.0;
    this.mInvI = 0.0;
    this.mMass = massData.mass;
    if (this.mMass <= 0.0) {
      this.mMass = 1.0;
    }
    this.mInvMass = 1.0 / this.mMass;
    if (massData.aI > 0.0 && (this.mFlags & Flags.fixedRotationFlag) === 0) {
      this.mI = massData.aI - this.mMass * b2Dot22(massData.center, massData.center);
      this.mInvI = 1.0 / this.mI;
    }
    let oldCenter = this.mSweep.c;
    this.mSweep.localCenter = massData.center;
    this.mSweep.c0 = b2MulT2(this.mXF, this.mSweep.localCenter);
    this.mSweep.c = this.mSweep.c0;
    addEqual(this.mLinearVelocity, b2Cross21(this.mAngularVelocity, subtract(this.mSweep.c, oldCenter)));
  }
  
  resetMassData(): void {
    this.mMass = 0.0;
    this.mInvMass = 0.0;
    this.mI = 0.0;
    this.mInvI = 0.0;
    this.mSweep.localCenter.setZero();
    if (this.mType === B2BodyType.STATICBODY || this.mType === B2BodyType.KINEMATICBODY) {
      this.mSweep.c0 = this.mXF.p;
      this.mSweep.c = this.mXF.p;
      this.mSweep.a0 = this.mSweep.a;
      return;
    }
    let localCenter = b2Vec2Zero;
    let f: B2Fixture | null = this.mFixtureList;
    while (f !== null) {
      if (f.mDensity === 0.0) {
        f = f!.mNext;
        continue;
      }
      let massData = f.massData;
      this.mMass += massData.mass;
      addEqual(localCenter, multM(massData.center, massData.mass));
      this.mI += massData.aI;
      f = f!.mNext;
    }
    if (this.mMass > 0.0) {
      this.mInvMass = 1.0 / this.mMass;
      mulEqual(localCenter, this.mInvMass);
    } else {
      this.mMass = 1.0;
      this.mInvMass = 1.0;
    }
    if (this.mI > 0.0 && (this.mFlags & Flags.fixedRotationFlag) === 0) {
      this.mI -= this.mMass * b2Dot22(localCenter, localCenter);
      this.mInvI = 1.0 / this.mI;
    } else {
      this.mI = 0.0;
      this.mInvI = 0.0;
    }
    let oldCenter = this.mSweep.c;
    this.mSweep.localCenter = localCenter;
    this.mSweep.c0 = b2MulT2(this.mXF, this.mSweep.localCenter);
    this.mSweep.c = this.mSweep.c0;
    addEqual(this.mLinearVelocity, b2Cross21(this.mAngularVelocity, subtract(this.mSweep.c, oldCenter)));
  }
  
  getWorldPoint(localPoint: B2Vec2): B2Vec2 {
    return b2MulT2(this.mXF, localPoint);
  }
  
  getWorldVector(localVector: B2Vec2): B2Vec2 {
    return b2MulR2(this.mXF.q, localVector);
  }
  
  getLocalPoint(worldPoint: B2Vec2): B2Vec2 {
    return b2MulTT2(this.mXF, worldPoint);
  }
  
  getLocalVector(worldVector: B2Vec2): B2Vec2 {
    return b2MulTR2(this.mXF.q, worldVector);
  }
  
  getLinearVelocityFromWorldPoint(worldPoint: B2Vec2): B2Vec2 {
    return add(this.mLinearVelocity, b2Cross21(this.mAngularVelocity, subtract(worldPoint, this.mSweep.c)));
  }
  
  getLinearVelocityFromLocalPoint(localPoint: B2Vec2): B2Vec2 {
    return this.getLinearVelocityFromWorldPoint(this.getWorldPoint(localPoint));
  }
  
  get linearDamping(): number {
    return this.mLinearDamping;
  }
  set linearDamping(newValue) {
    this.setLinearDamping(newValue);
  }
  
  setLinearDamping(linearDamping: number): void {
    this.mLinearDamping = linearDamping;
  }

  get angularDamping(): number {
    return this.mGravityScale;
  }
  set angularDamping(newValue) {
    this.setAngularDamping(newValue);
  }
  
  setAngularDamping(angularDamping: number): void {
    this.mAngularDamping = angularDamping;
  }

  get gravityScale(): number {
    return this.mGravityScale;
  }
  set gravityScale(newValue) {
    this.setGravityScale(newValue);
  }
  
  setGravityScale(scale: number): void {
    this.mGravityScale = scale;
  }
  
  setType(type: B2BodyType): void {
    if (this.mWorld.isLocked === true) {
      return;
    }
    if (this.mType === type) {
      return;
    }
    this.mType = type;
    this.resetMassData();
    if (this.mType === B2BodyType.STATICBODY) {
      this.mLinearVelocity.setZero();
      this.mAngularVelocity = 0.0;
      this.mSweep.a0 = this.mSweep.a;
      this.mSweep.c0 = this.mSweep.c;
      this.synchronizeFixtures();
    }
    this.setAwake(true);
    this.mForce.setZero();
    this.mTorque = 0.0;
    let ce: B2ContactEdge | null = this.mContactList;
    while (ce !== null) {
      let ce0 = ce!;
      ce = ce!.next;
      this.mWorld.mContactManager.destroy(ce0.contact);
    }
    this.mContactList = null;
    let broadPhase = this.mWorld.mContactManager.mBroadPhase;
    let f: B2Fixture | null = this.mFixtureList;
    while (f !== null) {
      let proxyCount = f!.mProxyCount;
      for (let i = 0; i < proxyCount; i++) {
        broadPhase.touchProxy(f!.mProxies[i].proxyId);
      }
      f = f!.mNext;
    }
  }

  get typeBody(): B2BodyType {
    return this.mType;
  }
  set typeBody(newValue) {
    this.setType(newValue);
  }
  
  setBullet(flag: boolean): void {
    if (flag) {
      this.mFlags |= Flags.bulletFlag;
    } else {
      this.mFlags &= ~Flags.bulletFlag;
    }
  }

  get isBullet(): boolean {
    return (this.mFlags & Flags.bulletFlag) === Flags.bulletFlag;
  }
  
  set isBullet(newValue) {
    this.setBullet(newValue);
  }
  
  setSleepingAllowed(flag: boolean): void {
    if (flag) {
      this.mFlags |= Flags.autoSleepFlag;
    } else {
      this.mFlags &= ~Flags.autoSleepFlag;
      this.setAwake(true);
    }
  }

  get isSleepingAllowed(): boolean {
    return (this.mFlags & Flags.autoSleepFlag) === Flags.autoSleepFlag;
  }
  
  set isSleepingAllowed(newValue) {
    this.setSleepingAllowed(newValue);
  }
  
  setAwake(flag: boolean): void {
    if (flag) {
      if ((this.mFlags & Flags.awakeFlag) === 0) {
        this.mFlags |= Flags.awakeFlag;
        this.mSleepTime = 0.0;
      }
    } else {
      this.mFlags &= ~Flags.awakeFlag;
      this.mSleepTime = 0.0;
      this.mLinearVelocity.setZero();
      this.mAngularVelocity = 0.0;
      this.mForce.setZero();
      this.mTorque = 0.0;
    }
  }
  
  get isAwake(): boolean {
    return (this.mFlags & Flags.awakeFlag) === Flags.awakeFlag;
  }
  
  setActive(flag: boolean): void {
    if (flag === this.isActive) {
      return;
    }
    if (flag) {
      this.mFlags |= Flags.activeFlag;
      let broadPhase = this.mWorld.mContactManager.mBroadPhase;
      let f: B2Fixture | null = this.mFixtureList;
      while (f !== null) {
        f!.createProxies(broadPhase, this.mXF);
        f = f!.mNext;
      }
    } else {
      this.mFlags &= ~Flags.activeFlag;
      let broadPhase = this.mWorld.mContactManager.mBroadPhase;
      let f: B2Fixture | null = this.mFixtureList;
      while (f !== null) {
        f!.destroyProxies(broadPhase);
        if (f!.mNext !== null) {
          f = f!.mNext;
        } else {
          break;
        }
      }
      let ce = this.mContactList;
      while (ce !== null) {
        let ce0 = ce!;
        ce = ce!.next;
        this.mWorld.mContactManager.destroy(ce0.contact);
      }
      this.mContactList = null;
    }
  }
  get isActive(): boolean {
    return (this.mFlags & Flags.activeFlag) === Flags.activeFlag;
  }
  
  setFixedRotation(flag: boolean): void {
    let status = (this.mFlags & Flags.fixedRotationFlag) === Flags.fixedRotationFlag;
    if (status === flag) {
      return;
    }
    if (flag) {
      this.mFlags |= Flags.fixedRotationFlag;
    } else {
      this.mFlags &= ~Flags.fixedRotationFlag;
    }
    this.mAngularVelocity = 0.0;
    this.resetMassData();
  }
  
  public isFixedRotation(): boolean {
    return (this.mFlags & Flags.fixedRotationFlag) === Flags.fixedRotationFlag;
  }
  
  public getFixtureList(): B2Fixture | null {
    return this.mFixtureList;
  }
  
  public getJointList(): B2JointEdge | null {
    return this.mJointList;
  }
  
  public getContactList(): B2ContactEdge | null {
    return this.mContactList;
  }
  
  public getNext(): B2Body | null {
    return this.mNext;
  }
  
  get userData(): object | null {
    return this.mUserData;
  }
  
  set userData(newValue) {
    this.setUserData(newValue);
  }
  
  public setUserData(data: object | null): void {
    this.mUserData = data;
  }
  
  get world(): B2World | null {
    return this.mWorld;
  }

  constructor(def: B2BodyDef, world: B2World) {
    this.mFlags = 0;
    if (def.bullet) {
      this.mFlags |= Flags.bulletFlag;
    }
    if (def.fixedRotation) {
      this.mFlags |= Flags.fixedRotationFlag;
    }
    if (def.allowSleep) {
      this.mFlags |= Flags.autoSleepFlag;
    }
    if (def.awake) {
      this.mFlags |= Flags.awakeFlag;
    }
    if (def.active) {
      this.mFlags |= Flags.activeFlag;
    }
    this.mWorld = world;
    this.mXF = new B2Transform();
    this.mXF.p = def.position;
    this.mXF.q.set(def.angle);
    this.mSweep = new B2Sweep();
    this.mSweep.localCenter = new B2Vec2(0.0, 0.0);
    this.mSweep.c0 = this.mXF.p;
    this.mSweep.c = this.mXF.p;
    this.mSweep.a0 = def.angle;
    this.mSweep.a = def.angle;
    this.mSweep.alpha0 = 0.0;
    this.mPrev = null;
    this.mNext = null;
    this.mLinearVelocity = def.linearVelocity;
    this.mAngularVelocity = def.angularVelocity;
    this.mLinearDamping = def.linearDamping;
    this.mAngularDamping = def.angularDamping;
    this.mGravityScale = def.gravityScale;
    this.mForce = new B2Vec2(0.0, 0.0);
    this.mTorque = 0.0;
    this.mSleepTime = 0.0;
    this.mType = def.type;
    if (this.mType === B2BodyType.DYNAMICBODY) {
      this.mMass = 1.0;
      this.mInvMass = 1.0;
    } else {
      this.mMass = 0.0;
      this.mInvMass = 0.0;
    }
    this.mI = 0.0;
    this.mInvI = 0.0;
    this.mUserData = def.userData!;
    this.mFixtureList = null;
    this.mFixtureCount = 0;
  }

  synchronizeFixtures(): void {
    let xf1 = new B2Transform();
    xf1.q.set(this.mSweep.a0);
    xf1.p = subtract(this.mSweep.c0, b2MulR2(xf1.q, this.mSweep.localCenter));
    let broadPhase = this.mWorld.mContactManager.mBroadPhase;
    let f: B2Fixture | null = this.mFixtureList;
    while (f !== null) {
      f!.synchronize(broadPhase, xf1, this.mXF);
      f = f!.mNext;
    }
  }
  
  synchronizeTransform(): void {
    this.mXF.q.set(this.mSweep.a);
    this.mXF.p = subtract(this.mSweep.c, b2MulR2(this.mXF.q, this.mSweep.localCenter));
    let tVec = this.mSweep.localCenter;
    let tMat = new B2Mat22();
    tMat.setMat(this.mSweep.a);
    this.mXF.p.y = this.mSweep.c.y - (tMat.ex.y * tVec.x + tMat.ey.y * tVec.y);
  }
  
  shouldCollide(other: B2Body): boolean {
    if (this.mType !== B2BodyType.DYNAMICBODY && other.mType !== B2BodyType.DYNAMICBODY) {
      return false;
    }
    let jn: B2JointEdge | null = this.mJointList;
    while (jn !== null) {
      if (jn!.other === other) {
        if (jn!.joint.mCollideConnected === false) {
          return false;
        }
      }
      jn = jn!.next;
    }
    return true;
  }
  advance(alpha: number): void {
    this.mSweep.advance(alpha);
    this.mSweep.c = this.mSweep.c0;
    this.mSweep.a = this.mSweep.a0;
    this.mXF.q.set(this.mSweep.a);
    this.mXF.p = subtract(this.mSweep.c, b2MulR2(this.mXF.q, this.mSweep.localCenter));
  }
}
export class Flags {
  static islandFlag = hex1;
  static awakeFlag = hex2;
  static autoSleepFlag = hex4;
  static bulletFlag = hex8;
  static fixedRotationFlag = hex10;
  static activeFlag = hex20;
  static toiFlag = hex40;
}
export { B2BodyType };
