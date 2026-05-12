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
import { B2Vec2 } from '../Common/b2Math';
import { b2Dot22, multM, add, addEqual, mulEqual } from '../Common/b2Math';
import { B2Contact } from './Contacts/b2Contact';
import { B2ContactVelocityConstraint } from './Contacts/b2ContactSolver';
import { B2ContactSolver, B2ContactSolverDef } from './Contacts/b2ContactSolver';
import { B2Joint } from './Joints/b2Joint';
import { B2Body } from './b2Body';
import { B2BodyType, Flags } from './b2Body';
import { B2TimeStep } from './b2TimeStep';
import { B2Array, B2Position, B2SolverData, B2Velocity } from './b2TimeStep';
import { B2ContactListener } from './b2WorldCallbacks';
import { B2ContactImpulse } from './b2WorldCallbacks';
import {
  b2AngularSleepTolerance,
  b2LinearSleepTolerance,
  b2MaxFloat,
  b2MaxRotation,
  b2MaxRotationSquared,
  b2MaxTranslation,
  b2MaxTranslationSquared,
  b2TimeToSleep
} from '../Common/b2Settings';

export class B2Island {
  mListener: B2ContactListener;
  mBodies: Array<B2Body>;
  mContacts: Array<B2Contact>;
  mJoints: Array<B2Joint>;
  mPositions: B2Array<B2Position>;
  mVelocities: B2Array<B2Velocity>;
  get mBodyCount(): number {
    return this.mBodies.length;
  }
  get mJointCount(): number {
    return this.mJoints.length;
  }
  get mContactCount(): number {
    return this.mContacts.length;
  }
  mBodyCapacity: number;
  mContactCapacity: number;
  mJointCapacity: number;
  constructor(bodyCapacity: number, contactCapacity: number, jointCapacity: number, listener: B2ContactListener) {
    this.mBodyCapacity = bodyCapacity;
    this.mContactCapacity = contactCapacity;
    this.mJointCapacity = jointCapacity;
    this.mListener = listener;
    this.mBodies = Array<B2Body>();
    this.mContacts = Array<B2Contact>();
    this.mJoints = Array<B2Joint>();
    this.mVelocities = new B2Array<B2Velocity>();
    this.mPositions = new B2Array<B2Position>();
  }

  reset(bodyCapacity: number, contactCapacity: number, jointCapacity: number, listener: B2ContactListener): void {
    this.mBodyCapacity = bodyCapacity;
    this.mContactCapacity = contactCapacity;
    this.mJointCapacity = jointCapacity;
    this.mListener = listener;
    this.mBodies.splice(0, this.mBodies.length);
    this.mContacts.splice(0, this.mContacts.length);
    this.mJoints.splice(0, this.mJoints.length);
    this.mVelocities.removeAll();
    this.mPositions.removeAll();
  }

  clear(): void {
    this.mBodies.splice(0, this.mBodies.length);
    this.mContacts.splice(0, this.mContacts.length);
    this.mJoints.splice(0, this.mJoints.length);
  }

  solve(step: B2TimeStep, gravity: B2Vec2, allowSleep: boolean): void {
    let h = step.dt;
    this.mPositions.removeAll();
    this.mVelocities.removeAll();
    for (let i = 0; i < this.mBodyCount; i++) {
      let b = this.mBodies[i];
      let c = b.mSweep.c;
      let a = b.mSweep.a;
      let v = b.mLinearVelocity;
      let w = b.mAngularVelocity;
      b.mSweep.c0 = b.mSweep.c;
      b.mSweep.a0 = b.mSweep.a;
      if (b.mType === B2BodyType.DYNAMICBODY) {
        addEqual(v, multM(add(multM(gravity, b.mGravityScale), multM(b.mForce, b.mInvMass)), h));
        w += h * b.mInvI * b.mTorque;
        mulEqual(v, 1.0 / (1.0 + h * b.mLinearDamping));
        w *= 1.0 / (1.0 + h * b.mAngularDamping);
      }
      this.mPositions.append(new B2Position(c, a));
      this.mVelocities.append(new B2Velocity(v, w));
    }
    let solverData = new B2SolverData();
    solverData.step = step;
    solverData.positions = this.mPositions;
    solverData.velocities = this.mVelocities;
    let contactSolverDef = new B2ContactSolverDef();
    contactSolverDef.step = step;
    contactSolverDef.contacts = this.mContacts;
    contactSolverDef.count = this.mContactCount;
    contactSolverDef.positions = this.mPositions;
    contactSolverDef.velocities = this.mVelocities;
    let contactSolver = new B2ContactSolver(contactSolverDef);
    contactSolver.initializeVelocityConstraints();
    if (step.warmStarting) {
      contactSolver.warmStart();
    }
    for (let i = 0; i < this.mJointCount; i++) {
      this.mJoints[i].initVelocityConstraints(solverData);
    }
    for (let i = 0; i < step.velocityIterations; i++) {
      for (let j = 0; j < this.mJointCount; j++) {
        this.mJoints[j].solveVelocityConstraints(solverData);
      }
      contactSolver.solveVelocityConstraints();
    }
    contactSolver.storeImpulses();
    for (let i = 0; i < this.mBodyCount; i++) {
      let c: B2Vec2 = this.mPositions.get(i).c;
      let a: number = this.mPositions.get(i).a;
      let v: B2Vec2 = this.mVelocities.get(i).v;
      let w: number = this.mVelocities.get(i).w;
      let translation = multM(v, h);
      if (b2Dot22(translation, translation) > b2MaxTranslationSquared) {
        let ratio = b2MaxTranslation / translation.length();
        mulEqual(v, ratio);
      }
      let rotation = h * w;
      if (rotation * rotation > b2MaxRotationSquared) {
        let ratio = b2MaxRotation / Math.abs(rotation);
        w *= ratio;
      }
      c = add(c, multM(v, h));
      a += h * w;
      this.mPositions.get(i).c = c;
      this.mPositions.get(i).a = a;
      this.mVelocities.get(i).v = v;
      this.mVelocities.get(i).w = w;
      let b = this.mBodies[i];
      b.mSweep.c0.set(b.mSweep.c.x, b.mSweep.c.y);
      b.mSweep.a0 = b.mSweep.a;
      b.mSweep.c.x += step.dt * b.mLinearVelocity.x;
      b.mSweep.c.y += step.dt * b.linearVelocity.y;
      b.mSweep.a += step.dt * b.angularVelocity;
      b.synchronizeTransform();
    }
    let positionSolved = false;
    for (let i = 0; i < step.positionIterations; i++) {
      let contactsOkay = contactSolver.solvePositionConstraints();
      let jointsOkay = true;
      for (let i2 = 0; i2 < this.mJointCount; i2++) {
        let jointOkay = this.mJoints[i2].solvePositionConstraints(solverData);
        jointsOkay = jointsOkay && jointOkay;
      }
      if (contactsOkay && jointsOkay) {
        positionSolved = true;
        break;
      }
    }
    for (let i = 0; i < this.mBodyCount; i++) {
      let body = this.mBodies[i];
      body.mSweep.c = this.mPositions.get(i).c;
      body.mSweep.a = this.mPositions.get(i).a;
      body.mLinearVelocity = this.mVelocities.get(i).v;
      body.mAngularVelocity = this.mVelocities.get(i).w;
      body.synchronizeTransform();
    }
    this.report(contactSolver.mVelocityConstraints);
    if (allowSleep) {
      let minSleepTime = b2MaxFloat;
      let linTolSqr = b2LinearSleepTolerance * b2LinearSleepTolerance;
      let angTolSqr = b2AngularSleepTolerance * b2AngularSleepTolerance;
      for (let i = 0; i < this.mBodyCount; i++) {
        let b = this.mBodies[i];
        if (b.typeBody === B2BodyType.STATICBODY) {
          continue;
        }
        let ats = b.mAngularVelocity * b.mAngularVelocity > angTolSqr;
        let lts = b2Dot22(b.mLinearVelocity, b.mLinearVelocity) > linTolSqr;
        if ((b.mFlags & Flags.autoSleepFlag) === 0 || ats || lts) {
          b.mSleepTime = 0.0;
          minSleepTime = 0.0;
        } else {
          b.mSleepTime += h;
          minSleepTime = Math.min(minSleepTime, b.mSleepTime);
        }
      }
      if (minSleepTime >= b2TimeToSleep && positionSolved) {
        for (let i = 0; i < this.mBodyCount; i++) {
          let b = this.mBodies[i];
          b.setAwake(false);
        }
      }
    }
  }

  solveToi(subStep: B2TimeStep, toiIndexA: number, toiIndexB: number): void {
    this.mPositions.removeAll();
    this.mVelocities.removeAll();
    for (let i = 0; i < this.mBodyCount; i++) {
      let b = this.mBodies[i];
      this.mPositions.append(new B2Position(b.mSweep.c, b.mSweep.a));
      this.mVelocities.append(new B2Velocity(b.mLinearVelocity, b.mAngularVelocity));
    }
    let contactSolverDef = new B2ContactSolverDef();
    contactSolverDef.contacts = this.mContacts;
    contactSolverDef.count = this.mContactCount;
    contactSolverDef.step = subStep;
    contactSolverDef.positions = this.mPositions;
    contactSolverDef.velocities = this.mVelocities;
    let contactSolver = new B2ContactSolver(contactSolverDef);
    for (let i = 0; i < subStep.positionIterations; i++) {
      let contactsOkay = contactSolver.solveToiPositionConstraints(toiIndexA, toiIndexB);
      if (contactsOkay) {
        break;
      }
    }
    this.mBodies[toiIndexA].mSweep.c0 = this.mPositions.get(toiIndexA).c;
    this.mBodies[toiIndexA].mSweep.a0 = this.mPositions.get(toiIndexA).a;
    this.mBodies[toiIndexB].mSweep.c0 = this.mPositions.get(toiIndexB).c;
    this.mBodies[toiIndexB].mSweep.a0 = this.mPositions.get(toiIndexB).a;
    contactSolver.initializeVelocityConstraints();
    for (let i = 0; i < subStep.velocityIterations; i++) {
      contactSolver.solveVelocityConstraints();
    }
    let h = subStep.dt;
    for (let i = 0; i < this.mBodyCount; i++) {
      let c: B2Vec2 = this.mPositions.get(i).c;
      let a: number = this.mPositions.get(i).a;
      let v: B2Vec2 = this.mVelocities.get(i).v;
      let w: number = this.mVelocities.get(i).w;
      let translation = multM(v, h);
      if (b2Dot22(translation, translation) > b2MaxTranslationSquared) {
        let ratio = b2MaxTranslation / translation.length();
        mulEqual(v, ratio);
      }
      let rotation = h * w;
      if (rotation * rotation > b2MaxRotationSquared) {
        let ratio = b2MaxRotation / Math.abs(rotation);
        w *= ratio;
      }
      addEqual(c, multM(v, h));
      a += h * w;
      this.mPositions.get(i).c = c;
      this.mPositions.get(i).a = a;
      this.mVelocities.get(i).v = v;
      this.mVelocities.get(i).w = w;
      let body = this.mBodies[i];
      body.mSweep.c = c;
      body.mSweep.a = a;
      body.mLinearVelocity = v;
      body.mAngularVelocity = w;
      body.synchronizeTransform();
    }
    this.report(contactSolver.mVelocityConstraints);
  }

  addB(body: B2Body): void {
    body.mIslandIndex = this.mBodyCount;
    this.mBodies.push(body);
  }

  addC(contact: B2Contact): void {
    this.mContacts.push(contact);
  }

  addJ(joint: B2Joint): void {
    this.mJoints.push(joint);
  }
  report(constraints: Array<B2ContactVelocityConstraint>): void {
    if (this.mListener === null) {
      return;
    }
    for (let i = 0; i < this.mContactCount; i++) {
      let c = this.mContacts[i];
      let vc = constraints[i];
      let impulse = new B2ContactImpulse();
      impulse.count = vc.pointCount;
      for (let j = 0; j < vc.pointCount; j++) {
        impulse.normalImpulses[j] = vc.points[j].normalImpulse;
        impulse.tangentImpulses[j] = vc.points[j].tangentImpulse;
      }
      this.mListener?.postSolve(c, impulse);
    }
  }
}
