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
import { B2BroadPhase } from '../Collision/b2BroadPhase';
import { B2AaBb } from '../Collision/b2Collision';
import { B2RayCastInput, B2RayCastOutput } from '../Collision/b2Collision';
import { B2ToiInput, B2ToiOutput, b2TimeOfImpact, State } from '../Collision/b2TimeOfImpact';
import { B2Vec2 } from '../Common/b2Math';
import { multM, add, subtractEqual } from '../Common/b2Math';
import { B2QueryWrapper, B2RayCastWrapper } from '../Common/b2Wrappers';
import { B2Island } from './b2Island';
import { B2BodyDef } from './b2Body';
import { Flags, B2Body, B2BodyType } from './b2Body';
import { B2Joint } from './Joints/b2Joint';
import { B2TimeStep } from './b2TimeStep';
import {
  B2QueryCallbackFunction,
  B2ContactFilter,
  B2RayCastCallback,
  B2DestructionListener,
  B2RayCastCallbackFunction,
  B2ContactListener,
  B2QueryCallback
} from './b2WorldCallbacks';
import { B2QueryCallbackProxy, B2RayCastCallbackProxy } from './b2WorldCallbacks';
import { B2Contact } from './Contacts/b2Contact';
import { FlagContacts } from './Contacts/b2Contact';
import { B2Profile } from './b2TimeStep';
import { B2ContactManager } from './b2ContactManager';
import { b2Epsilon, b2MaxSubSteps, b2MaxTOIContacts, hex2, hex4, float10, indexTwo, posIter, hex1 } from '../Common/b2Settings';

export class B2World {
  mIsland: B2Island | null = null;
  mToiIsland: B2Island | null = null;
  mFlags = FlagsW.clearForces;
  mContactManager: B2ContactManager = new B2ContactManager();
  mBodyList: B2Body | null = null;
  mJointList: B2Joint | null = null;
  mBodyCount: number = 0;
  mJointCount: number = 0;
  mGravity: B2Vec2;
  mAllowSleep: boolean = true;
  mDestructionListener: B2DestructionListener | null = null;
  mInvDt0: number = 0.0;
  mWarmStarting: boolean = true;
  mContinuousPhysics: boolean = true;
  mSubStepping: boolean = false;
  mStepComplete: boolean = true;
  mProfile: B2Profile = new B2Profile();
  constructor(gravity: B2Vec2) {
    this.mDestructionListener = null;
    this.mBodyList = null;
    this.mJointList = null;
    this.mBodyCount = 0;
    this.mJointCount = 0;
    this.mWarmStarting = true;
    this.mContinuousPhysics = true;
    this.mSubStepping = false;
    this.mStepComplete = true;
    this.mAllowSleep = true;
    this.mGravity = gravity;
    this.mFlags = FlagsW.clearForces;
    this.mInvDt0 = 0.0;
    this.mContactManager = new B2ContactManager();
  }

  setDestructionListener(listener: B2DestructionListener): void {
    this.mDestructionListener = listener;
  }

  setContactFilter(filter: B2ContactFilter): void {
    this.mContactManager.mContactFilter = filter;
  }

  setContactListener(listener: B2ContactListener): void {
    this.mContactManager.mContactListener = listener;
  }

  public createBody(def: B2BodyDef): B2Body {
    if (this.isLocked) {
      throw new Error('world is locked');
    }
    let b = new B2Body(def, this);
    b.mPrev = null;
    b.mNext = this.mBodyList;
    if (this.mBodyList !== null) {
      this.mBodyList.mPrev = b;
    }
    this.mBodyList = b;
    this.mBodyCount += 1;
    return b;
  }

  public destroyBody(b: B2Body): void {
    if (this.isLocked) {
      return;
    }
    let je = b.mJointList;
    while (je !== null) {
      let je0 = je!;
      je = je.next;
      if (this.mDestructionListener !== null) {
        this.mDestructionListener.sayGoodbye(je0.joint);
      }
      b.mJointList = je;
    }
    b.mJointList = null;
    let ce = b.mContactList;
    while (ce !== null) {
      let ce0 = ce!;
      ce = ce.next;
      this.mContactManager.destroy(ce0.contact);
    }
    b.mContactList = null;
    let f = b.mFixtureList;
    while (f !== null) {
      let f0 = f!;
      f = f.mNext!;
      if (this.mDestructionListener !== null) {
        this.mDestructionListener.sayGoodbye(f0);
      }
      f0.destroyProxies(this.mContactManager.mBroadPhase);
      f0.destroy();
      b.mFixtureList = f;
      b.mFixtureCount -= 1;
    }
    b.mFixtureList = null;
    b.mFixtureCount = 0;
    if (b.mPrev !== null) {
      b.mPrev.mNext = b.mNext;
    }
    if (b.mNext !== null) {
      b.mNext.mPrev = b.mPrev;
    }
    if (b === this.mBodyList) {
      this.mBodyList = b.mNext;
    }
    this.mBodyCount -= 1;
  }

  public step(dt: number, velocityIterations: number, positionIterations: number): void {
    if ((this.mFlags & FlagsW.newFixture) !== 0) {
      this.mContactManager.findNewContacts();
      this.mFlags &= ~FlagsW.newFixture;
    }
    this.mFlags |= FlagsW.locked;
    let step = new B2TimeStep();
    step.dt = dt;
    step.velocityIterations = velocityIterations;
    step.positionIterations = positionIterations;
    if (dt > 0.0) {
      step.invDt = 1.0 / dt;
    } else {
      step.invDt = 0.0;
    }
    step.dtRatio = this.mInvDt0 * dt;
    step.warmStarting = this.mWarmStarting;
    this.mContactManager.collide();
    if (this.mStepComplete && step.dt > 0.0) {
      this.solve(step);
    }
    if (this.mContinuousPhysics && step.dt > 0.0) {
      this.solveToi(step);
    }
    if (step.dt > 0.0) {
      this.mInvDt0 = step.invDt;
    }
    if ((this.mFlags & FlagsW.clearForces) !== 0) {
      this.clearForces();
    }
    this.mFlags &= ~FlagsW.locked;
  }

  public clearForces(): void {
    let body = this.mBodyList;
    while (body !== null) {
      body.mForce.setZero();
      body.mTorque = 0.0;
      body = body.getNext();
    }
  }

  public queryAaBb(callback: B2QueryCallback, aabb: B2AaBb): void {
    let wrapper = new B2WorldQueryWrapper();
    wrapper.broadPhase = this.mContactManager.mBroadPhase;
    wrapper.callback = callback;
    this.mContactManager.mBroadPhase.query(wrapper, aabb);
  }

  public queryAaBb2(aabb: B2AaBb, callback: B2QueryCallbackFunction): void {
    this.queryAaBb(new B2QueryCallbackProxy(callback), aabb);
  }

  public rayCast(callback: B2RayCastCallback, point1: B2Vec2, point2: B2Vec2): void {
    let wrapper = new B2WorldRayCastWrapper();
    wrapper.broadPhase = this.mContactManager.mBroadPhase;
    wrapper.callback = callback;
    let input = new B2RayCastInput();
    input.maxFraction = 1.0;
    input.p1 = point1;
    input.p2 = point2;
    this.mContactManager.mBroadPhase.rayCast(wrapper, input);
  }

  public rayCast2(point1: B2Vec2, point2: B2Vec2, callback: B2RayCastCallbackFunction): void {
    this.rayCast(new B2RayCastCallbackProxy(callback), point1, point2);
  }

  public getBodyList(): B2Body | null {
    return this.mBodyList;
  }

  public getJointList(): B2Joint | null {
    return this.mJointList;
  }

  public getContactList(): B2Contact | null {
    return this.mContactManager.mContactList;
  }

  public setAllowSleeping(flag: boolean): void {
    if (flag === this.mAllowSleep) {
      return;
    }
    this.mAllowSleep = flag;
    if (this.mAllowSleep === false) {
      let b = this.mBodyList;
      while (b !== null) {
        b.setAwake(true);
        b = b.mNext;
      }
    }
  }

  public get allowSleeping(): boolean {
    return this.mAllowSleep;
  }

  public set allowSleeping(newValue: boolean) {
    this.setAllowSleeping(newValue);
  }

  public setWarmStarting(flag: boolean): void {
    this.mWarmStarting = flag;
  }

  public get warmStarting(): boolean {
    return this.mWarmStarting;
  }

  public set warmStarting(newValue: boolean) {
    this.setWarmStarting(newValue);
  }

  public setContinuousPhysics(flag: boolean): void {
    this.mContinuousPhysics = flag;
  }

  public get continuousPhysics(): boolean {
    return this.mContinuousPhysics;
  }

  public set continuousPhysics(newValue: boolean) {
    this.setContinuousPhysics(newValue);
  }

  public setSubStepping(flag: boolean): void {
    this.mSubStepping = flag;
  }

  public get subStepping(): boolean {
    return this.mSubStepping;
  }

  public set subStepping(newValue: boolean) {
    this.setSubStepping(newValue);
  }

  public get proxyCount(): number {
    return this.mContactManager.mBroadPhase.getProxyCount();
  }

  public get bodyCount(): number {
    return this.mBodyCount;
  }

  public get jointCount(): number {
    return this.mJointCount;
  }

  public get contactCount(): number {
    return this.mContactManager.mContactCount;
  }

  public get treeHeight(): number {
    return this.mContactManager.mBroadPhase.getTreeHeight();
  }

  public get treeBalance(): number {
    return this.mContactManager.mBroadPhase.getTreeBalance();
  }

  public get treeQuality(): number {
    return this.mContactManager.mBroadPhase.getTreeQuality();
  }

  public setGravity(gravity: B2Vec2): void {
    this.mGravity = gravity;
  }

  public get gravity(): B2Vec2 {
    return this.mGravity;
  }

  public set gravity(newValue: B2Vec2) {
    this.setGravity(newValue);
  }

  public get isLocked(): boolean {
    return (this.mFlags & FlagsW.locked) === FlagsW.locked;
  }

  public setAutoClearForces(flag: boolean): void {
    if (flag) {
      this.mFlags |= FlagsW.clearForces;
    } else {
      this.mFlags &= ~FlagsW.clearForces;
    }
  }

  public get autoClearForces(): boolean {
    return (this.mFlags & FlagsW.clearForces) === FlagsW.clearForces;
  }

  public shiftOrigin(newOrigin: B2Vec2): void {
    if ((this.mFlags & FlagsW.locked) === FlagsW.locked) {
      return;
    }
    let b = this.mBodyList;
    while (b !== null) {
      subtractEqual(b.mXF.p, newOrigin);
      subtractEqual(b.mSweep.c0, newOrigin);
      subtractEqual(b.mSweep.c, newOrigin);
      b = b.mNext;
    }
    let j = this.mJointList;
    while (j !== null) {
      j.shiftOrigin(newOrigin);
      j = j.mNext;
    }
    this.mContactManager.mBroadPhase.shiftOrigin(newOrigin);
  }

  public get contactManager(): B2ContactManager {
    return this.mContactManager;
  }

  public get profile(): B2Profile {
    return this.mProfile;
  }

  solve(step: B2TimeStep): void {
    this.mProfile.solveInit = 0.0;
    this.mProfile.solveVelocity = 0.0;
    this.mProfile.solvePosition = 0.0;

    if (this.mIsland === null) {
      this.mIsland = new B2Island(this.mBodyCount, this.mContactManager.mContactCount, this.mJointCount, this.mContactManager.mContactListener!);
    } else {
      this.mIsland.reset(this.mBodyCount, this.mContactManager.mContactCount, this.mJointCount, this.mContactManager.mContactListener!);
    }
    let island = this.mIsland;
    let b = this.mBodyList;
    while (b !== null) {
      b.mFlags &= ~Flags.islandFlag;
      b = b.mNext;
    }
    let c = this.mContactManager.mContactList;
    while (c !== null) {
      c.mFlags &= ~FlagContacts.ISLANDFLAG;
      c = c.mNext;
    }
    let j = this.mJointList;
    while (j !== null) {
      j.mIslandFlag = false;
      j = j.mNext;
    }
    let stack = new Array<B2Body>();
    let seed = this.mBodyList;
    while (seed !== null) {
      if ((seed.mFlags & Flags.islandFlag) !== 0) {
        seed = seed.mNext;
        continue;
      }
      if (seed!.isAwake === false || seed!.isActive === false) {
        seed = seed!.mNext;
        continue;
      }
      if (seed!.typeBody === B2BodyType.STATICBODY) {
        seed = seed!.mNext;
        continue;
      }
      island.clear();
      stack.push(seed!);
      seed!.mFlags |= Flags.islandFlag;
      while (stack.length > 0) {
        let b = stack.pop();
        island.addB(b!);
        b!.setAwake(true);
        if (b!.typeBody === B2BodyType.STATICBODY) {
          continue;
        }
        let ce = b!.mContactList;
        while (ce !== null) {
          let contact = ce!.contact;
          if ((contact.mFlags & FlagContacts.ISLANDFLAG) !== 0) {
            ce = ce!.next;
            continue;
          }
          if (contact.isEnabled === false || contact.isTouching === false) {
            ce = ce!.next;
            continue;
          }
          let sensorA = contact.mFixtureA.mIsSensor;
          let sensorB = contact.mFixtureB.mIsSensor;
          if (sensorA || sensorB) {
            ce = ce!.next;
            continue;
          }
          island?.addC(contact);
          contact.mFlags |= FlagContacts.ISLANDFLAG;
          let other = ce!.other;
          if ((other !== null) && (other.mFlags & Flags.islandFlag) !== 0) {
            ce = ce!.next;
            continue;
          }
          stack.push(other!);
          if (other !== null) {
            other.mFlags |= Flags.islandFlag;
          }
          ce = ce!.next;
        }
        let je = b!.mJointList;
        while (je !== null) {
          if (je!.joint.mIslandFlag === true) {
            je = je!.next;
            continue;
          }
          let other: B2Body = je.other!;
          if (other.isActive === false) {
            je = je.next;
            continue;
          }
          island?.addJ(je.joint);
          je!.joint.mIslandFlag = true;
          if ((other.mFlags & Flags.islandFlag) !== 0) {
            je = je!.next;
            continue;
          }
          stack.push(other);
          other.mFlags |= Flags.islandFlag;
        }
      }
      island.solve(step, this.mGravity, this.mAllowSleep);
      for (let i = 0; i < island.mBodyCount; i++) {
        let b = island.mBodies[i];
        if (b.typeBody === B2BodyType.STATICBODY) {
          b.mFlags &= ~Flags.islandFlag;
        }
      }
      seed = seed!.mNext;
    }
    stack.splice(0, stack.length);
    let locB = this.mBodyList;
    while (locB !== null) {
      if ((locB!.mFlags & Flags.islandFlag) === 0) {
        locB = locB!.getNext();
        continue;
      }
      if (locB!.typeBody === B2BodyType.STATICBODY) {
        locB = locB!.getNext();
        continue;
      }
      locB!.synchronizeFixtures();
      locB = locB!.getNext();
    }
    this.mContactManager.findNewContacts();
  }

  solveToi(step: B2TimeStep): void {
    if (this.mToiIsland === null) {
      this.mToiIsland = new B2Island(indexTwo * b2MaxTOIContacts, b2MaxTOIContacts, 0, this.mContactManager.mContactListener!);
    } else {
      this.mToiIsland.reset(indexTwo * b2MaxTOIContacts, b2MaxTOIContacts, 0, this.mContactManager.mContactListener!);
    }
    let island = this.mToiIsland!;
    if (this.mStepComplete) {
      let b = this.mBodyList;
      while (b !== null) {
        b.mFlags &= ~Flags.islandFlag;
        b.mSweep.alpha0 = 0.0;
        b = b.mNext;
      }
      let c = this.mContactManager.mContactList;
      while (c !== null) {
        c.mFlags &= ~(FlagContacts.TOIFLAG | FlagContacts.ISLANDFLAG);
        c.mToiCount = 0;
        c.mToi = 1.0;
        c = c.mNext;
      }
    }
    while (true) {
      let minContact: B2Contact | null = null;
      let minAlpha: number = 1.0;
      let c = this.mContactManager.mContactList;
      while (c !== null) {
        if (c.isEnabled === false) {
          c = c!.mNext;
          continue;
        }
        if (c.mToiCount > b2MaxSubSteps) {
          c = c!.mNext;
          continue;
        }
        let alpha: number = 1.0;
        if ((c!.mFlags & FlagContacts.TOIFLAG) !== 0) {
          alpha = c!.mToi;
        } else {
          let fA = c!.fixtureA;
          let fB = c!.fixtureB;
          if (fA.isSensor || fB.isSensor) {
            c = c!.mNext;
            continue;
          }
          let bA = fA.body;
          let bB = fB.body;
          let typeA = bA.mType;
          let typeB = bB.mType;
          let activeA = bA.isAwake && typeA !== B2BodyType.STATICBODY;
          let activeB = bB.isAwake && typeB !== B2BodyType.STATICBODY;
          if (activeA === false && activeB === false) {
            c = c!.mNext;
            continue;
          }
          let collideA = bA.isBullet || typeA !== B2BodyType.DYNAMICBODY;
          let collideB = bB.isBullet || typeB !== B2BodyType.DYNAMICBODY;
          if (collideA === false && collideB === false) {
            c = c!.mNext;
            continue;
          }
          let alpha0 = bA.mSweep.alpha0;
          if (bA.mSweep.alpha0 < bB.mSweep.alpha0) {
            alpha0 = bB.mSweep.alpha0;
            bA.mSweep.advance(alpha0);
          } else if (bB.mSweep.alpha0 < bA.mSweep.alpha0) {
            alpha0 = bA.mSweep.alpha0;
            bB.mSweep.advance(alpha0);
          }
          let input = new B2ToiInput();
          input.proxyA.set(fA.shape!);
          input.proxyB.set(fB.shape!);
          input.sweepA = bA.mSweep;
          input.sweepB = bB.mSweep;
          input.tMax = 1.0;
          let output = new B2ToiOutput();
          b2TimeOfImpact(output, input);
          let beta = output.t;
          if (output.state === State.TOUCHING) {
            alpha = Math.min(alpha0 + (1.0 - alpha0) * beta, 1.0);
          } else {
            alpha = 1.0;
          }
          c!.mToi = alpha;
          c!.mFlags |= FlagContacts.TOIFLAG;
        }
        if (alpha < minAlpha) {
          minContact = c;
          minAlpha = alpha;
        }
        c = c!.mNext;
      }
      if (minContact === null || 1.0 - float10 * b2Epsilon < minAlpha) {
        this.mStepComplete = true;
        break;
      }
      let fA = minContact!.fixtureA;
      let fB = minContact!.fixtureB;
      let bA = fA.body;
      let bB = fB.body;
      let backup1 = bA.mSweep;
      let backup2 = bB.mSweep;
      bA.advance(minAlpha);
      bB.advance(minAlpha);
      minContact!.update(this.mContactManager.mContactListener!);
      minContact!.mFlags &= ~FlagContacts.TOIFLAG;
      minContact!.mToiCount += 1;
      if (minContact.isEnabled === false || minContact.isTouching === false) {
        minContact!.setEnabled(false);
        bA.mSweep = backup1;
        bB.mSweep = backup2;
        bA.synchronizeTransform();
        bB.synchronizeTransform();
        continue;
      }
      bA.setAwake(true);
      bB.setAwake(true);
      island.clear();
      island.addB(bA);
      island.addB(bB);
      island.addC(minContact);
      bA.mFlags |= Flags.islandFlag;
      bB.mFlags |= Flags.islandFlag;
      minContact.mFlags |= FlagContacts.ISLANDFLAG;
      let bodies = [bA, bB];
      for (let i = 0; i < indexTwo; i++) {
        let body = bodies[i];
        if (body.mType === B2BodyType.DYNAMICBODY) {
          let ce = body.mContactList;
          while (ce !== null) {
            if (island.mBodyCount === island.mBodyCapacity) {
              break;
            }
            if (island.mContactCount === island.mContactCapacity) {
              break;
            }
            let contact = ce!.contact;
            if ((contact.mFlags & FlagContacts.ISLANDFLAG) !== 0) {
              ce = ce!.next;
              continue;
            }
            let other = ce!.other!;
            if (other.mType === B2BodyType.DYNAMICBODY && body.isBullet === false && other.isBullet === false) {
              ce = ce!.next;
              continue;
            }
            let sensorA = contact.mFixtureA.mIsSensor;
            let sensorB = contact.mFixtureB.mIsSensor;
            if (sensorA || sensorB) {
              ce = ce!.next;
              continue;
            }
            let backup = other.mSweep;
            if ((other.mFlags & Flags.islandFlag) === 0) {
              other.advance(minAlpha);
            }
            contact.update(this.mContactManager.mContactListener!);
            if (contact.isEnabled === false) {
              other.mSweep = backup;
              other.synchronizeTransform();
              ce = ce!.next;
              continue;
            }
            if (contact.isTouching === false) {
              other.mSweep = backup;
              other.synchronizeTransform();
              ce = ce!.next;
              continue;
            }
            contact.mFlags |= FlagContacts.ISLANDFLAG;
            island.addC(contact);
            if ((other.mFlags & Flags.islandFlag) !== 0) {
              ce = ce!.next;
              continue;
            }
            other.mFlags |= Flags.islandFlag;
            if (other.mType !== B2BodyType.STATICBODY) {
              other.setAwake(true);
            }
            island.addB(other);
            ce = ce!.next;
          }
        }
      }
      let subStep = new B2TimeStep();
      subStep.dt = (1.0 - minAlpha) * step.dt;
      subStep.invDt = 1.0 / subStep.dt;
      subStep.dtRatio = 1.0;
      subStep.positionIterations = posIter;
      subStep.velocityIterations = step.velocityIterations;
      subStep.warmStarting = false;
      island.solveToi(subStep, bA.mIslandIndex, bB.mIslandIndex);
      for (let i = 0; i < island.mBodyCount; i++) {
        let body = island.mBodies[i];
        body.mFlags &= ~Flags.islandFlag;
        if (body.mType !== B2BodyType.DYNAMICBODY) {
          continue;
        }
        body.synchronizeFixtures();
        let ce = body.mContactList;
        while (ce !== null) {
          ce!.contact.mFlags &= ~(FlagContacts.TOIFLAG | FlagContacts.ISLANDFLAG);
          ce = ce!.next;
        }
      }
      this.mContactManager.findNewContacts();
      if (this.mSubStepping) {
        this.mStepComplete = false;
        break;
      }
    }
  }
}

export class FlagsW {
  static newFixture: number = hex1;
  static locked: number = hex2;
  static clearForces: number = hex4;
}
export class B2WorldQueryWrapper implements B2QueryWrapper {
  broadPhase: B2BroadPhase | null = null;
  callback: B2QueryCallback | null = null;
  queryCallback(proxyId: number): boolean {
    let proxy = this.broadPhase?.getUserData(proxyId)!;
    return this.callback!.reportFixture(proxy.fixture);
  }
}
export class B2WorldRayCastWrapper implements B2RayCastWrapper {
  broadPhase: B2BroadPhase | null = null;
  callback: B2RayCastCallback | null = null;
  rayCastCallback(input: B2RayCastInput, proxyId: number): number {
    let proxy = this.broadPhase?.getUserData(proxyId)!;
    let fixture = proxy.fixture;
    let index = proxy.childIndex;
    let output = new B2RayCastOutput();
    let hit = fixture.rayCast(output, input, index);
    if (hit) {
      let fraction = output.fraction;
      let point = add(multM(input.p1, 1.0 - fraction), multM(input.p2, fraction));
      return this.callback!.reportFixture(fixture, point, output.normal, fraction);
    }
    return input.maxFraction;
  }
}
