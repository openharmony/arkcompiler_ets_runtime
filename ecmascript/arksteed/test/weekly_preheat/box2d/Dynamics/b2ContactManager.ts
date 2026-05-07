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
import { B2ContactListener } from './b2WorldCallbacks';
import { B2ContactFilter, B2DefaultContactListener } from './b2WorldCallbacks';
import { B2Contact, FlagContacts } from './Contacts/b2Contact';
import { B2BroadPhaseWrapper } from '../Common/b2Wrappers';
import { B2FixtureProxy } from './b2Fixture';
import { B2BroadPhase } from '../Collision/b2BroadPhase';
import { B2BodyType } from './b2Body';

let b2DefaultFilter = new B2ContactFilter();
let b2DefaultListener = new B2DefaultContactListener();

export class B2ContactManager implements B2BroadPhaseWrapper {
  mContactList: B2Contact | null = null;
  mContactCount: number = 0;
  mContactFilter: B2ContactFilter | null = b2DefaultFilter;
  mContactListener: B2ContactListener | null = b2DefaultListener;
  constructor() {
    this.mContactList = null;
    this.mContactCount = 0;
    this.mContactFilter = b2DefaultFilter;
    this.mContactListener = b2DefaultListener;
  }

  public addPair(proxyUserDataA: B2FixtureProxy, proxyUserDataB: B2FixtureProxy): void {
    let proxyA = proxyUserDataA;
    let proxyB = proxyUserDataB;
    let fixtureA = proxyA.fixture;
    let fixtureB = proxyB.fixture;
    let indexA = proxyA.childIndex;
    let indexB = proxyB.childIndex;
    let bodyA = fixtureA.body;
    let bodyB = fixtureB.body;
    if (bodyA === bodyB) {
      return;
    }
    let edge = bodyB.getContactList();
    while (edge !== null) {
      if (edge.other === bodyA) {
        let fA = edge.contact.fixtureA;
        let fB = edge.contact.fixtureB;
        let iA = edge.contact.childIndexA;
        let iB = edge.contact.childIndexB;
        if (fA === fixtureA && fB === fixtureB && iA === indexA && iB === indexB) {
          return;
        }
        if (fA === fixtureB && fB === fixtureA && iA === indexB && iB === indexA) {
          return;
        }
      }
      edge = edge!.next;
    }
    if (bodyB.shouldCollide(bodyA) === false) {
      return;
    }
    if (this.mContactFilter !== null && this.mContactFilter.shouldCollide(fixtureA, fixtureB) === false) {
      return;
    }
    let c = B2Contact.create(fixtureA, indexA, fixtureB, indexB);
    if (c === null) {
      return;
    }
    fixtureA = c.fixtureA;
    fixtureB = c.fixtureB;
    indexA = c.childIndexA;
    indexB = c.childIndexB;
    bodyA = fixtureA.body;
    bodyB = fixtureB.body;
    c.mPrev = null;
    c.mNext = this.mContactList;
    if (this.mContactList !== null) {
      this.mContactList.mPrev = c;
    }
    this.mContactList = c;
    c.mNodeA.contact = c!;
    c.mNodeA.other = bodyB;
    c.mNodeA.prev = null;
    c.mNodeA.next = bodyA.mContactList;
    if (bodyA.mContactList !== null) {
      bodyA.mContactList.prev = c.mNodeA;
    }
    bodyA.mContactList = c.mNodeA;
    c.mNodeB.contact = c!;
    c.mNodeB.other = bodyA;
    c.mNodeB.prev = null;
    c.mNodeB.next = bodyB.mContactList;
    if (bodyB.mContactList !== null) {
      bodyB.mContactList.prev = c.mNodeB;
    }
    bodyB.mContactList = c.mNodeB;
    if (fixtureA.isSensor === false && fixtureB.isSensor === false) {
      bodyA.setAwake(true);
      bodyB.setAwake(true);
    }
    this.mContactCount += 1;
  }

  findNewContacts(): void {
    this.mBroadPhase.updatePairs(this);
  }

  destroy(c: B2Contact): void {
    let fixtureA = c.fixtureA;
    let fixtureB = c.fixtureB;
    let bodyA = fixtureA.body;
    let bodyB = fixtureB.body;
    if (this.mContactListener !== null && c.isTouching) {
      this.mContactListener.endContact(c);
    }
    if (c.mPrev !== null) {
      c.mPrev.mNext = c.mNext;
    }
    if (c.mNext !== null) {
      c.mNext.mPrev = c.mPrev;
    }
    if (c === this.mContactList) {
      this.mContactList = c.mNext;
    }
    if (c.mNodeA.prev !== null) {
      c.mNodeA.prev.next = c.mNodeA.next;
    }
    if (c.mNodeA.next !== null) {
      c.mNodeA.next.prev = c.mNodeA.prev;
    }
    if (c.mNodeA === bodyA.mContactList) {
      bodyA.mContactList = c.mNodeA.next;
    }
    if (c.mNodeB.prev !== null) {
      c.mNodeB.prev.next = c.mNodeB.next;
    }
    if (c.mNodeB.next !== null) {
      c.mNodeB.next.prev = c.mNodeB.prev;
    }
    if (c.mNodeB === bodyB.mContactList) {
      bodyB.mContactList = c.mNodeB.next;
    }
    B2Contact.destroy(c);
    this.mContactCount -= 1;
  }

  collide(): void {
    let c = this.mContactList;
    while (c !== null) {
      let fixtureA = c.fixtureA;
      let fixtureB = c.fixtureB;
      let indexA = c.childIndexA;
      let indexB = c.childIndexB;
      let bodyA = fixtureA.body;
      let bodyB = fixtureB.body;
      if ((c.mFlags & FlagContacts.FILTERFLAG) !== 0) {
        if (bodyB.shouldCollide(bodyA) === false) {
          let cNuke = c!;
          c = cNuke.getNext();
          this.destroy(cNuke);
          continue;
        }
        if (this.mContactFilter !== null && this.mContactFilter.shouldCollide(fixtureA, fixtureB) === false) {
          let cNuke = c!;
          c = cNuke.getNext();
          this.destroy(cNuke);
          continue;
        }
        c.mFlags &= ~FlagContacts.FILTERFLAG;
      }
      let activeA = bodyA.isAwake && bodyA.mType !== B2BodyType.STATICBODY;
      let activeB = bodyB.isAwake && bodyB.mType !== B2BodyType.STATICBODY;
      if (activeA === false && activeB === false) {
        c = c.getNext();
        continue;
      }
      let proxyIdA = fixtureA.mProxies[indexA].proxyId;
      let proxyIdB = fixtureB.mProxies[indexB].proxyId;
      let overlap = this.mBroadPhase.testOverlap(proxyIdA, proxyIdB);
      if (overlap === false) {
        let cNuke = c!;
        c = cNuke.getNext();
        this.destroy(cNuke);
        continue;
      }
      c.update(this.mContactListener!);
      c = c.getNext();
    }
  }
  mBroadPhase = new B2BroadPhase();
  public broadPhase(): B2BroadPhase {
    return this.mBroadPhase;
  }
}
