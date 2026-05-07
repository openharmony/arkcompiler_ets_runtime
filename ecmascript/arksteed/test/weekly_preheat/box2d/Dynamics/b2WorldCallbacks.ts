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
import { B2Joint } from './Joints/b2Joint';
import { B2Manifold } from '../Collision/b2Collision';
import { B2Contact } from './Contacts/b2Contact';
import { B2Fixture } from './b2Fixture';
import { b2MaxManifoldPoints } from '../Common/b2Settings';

interface B2DestructionListener {
  sayGoodbye(joint: B2Joint): void;
  sayGoodbye(fixture: B2Fixture): void;
}

export class B2ContactFilter {
  shouldCollide(fixtureA: B2Fixture, fixtureB: B2Fixture): boolean {
    let filterA = fixtureA.filterData;
    let filterB = fixtureB.filterData;
    if (filterA.groupIndex === filterB.groupIndex && filterA.groupIndex !== 0) {
      return filterA.groupIndex > 0;
    }
    let collide = (filterA.maskBits & filterB.categoryBits) !== 0 && (filterA.categoryBits & filterB.maskBits) !== 0;
    return collide;
  }
}

export class B2ContactImpulse {
  public normalImpulses: Array<number>;
  public tangentImpulses: Array<number>;
  public count: number = 0;

  constructor() {
    this.normalImpulses = [];
    this.tangentImpulses = [];
    for (let i = 0; i < b2MaxManifoldPoints; i++) {
      this.normalImpulses.push(0);
      this.tangentImpulses.push(0);
    }
  }
}

interface B2ContactListener {
  beginContact(contact: B2Contact): void;
  endContact(contact: B2Contact): void;
  preSolve(contact: B2Contact, oldManifold: B2Manifold): void;
  postSolve(contact: B2Contact, impulse: B2ContactImpulse): void;
}

export class B2DefaultContactListener implements B2ContactListener {
  beginContact(contact: B2Contact):void {}
  endContact(contact: B2Contact):void {}
  preSolve(contact: B2Contact, oldManifold: B2Manifold):void {}
  postSolve(contact: B2Contact, impulse: B2ContactImpulse):void {}
}

interface B2QueryCallback {
  reportFixture(fixture: B2Fixture): boolean;
}

interface B2QueryCallbackFunction {
  queryBack(fixture: B2Fixture): boolean;
}

export class B2QueryCallbackProxy implements B2QueryCallback {
  callback: B2QueryCallbackFunction;
  constructor(callback: B2QueryCallbackFunction) {
    this.callback = callback;
  }
  reportFixture(fixture: B2Fixture): boolean {
    return this.callback.queryBack(fixture);
  }
}

interface B2RayCastCallback {
  reportFixture(fixture: B2Fixture, point: B2Vec2, normal: B2Vec2, fraction: number): number;
}

interface B2RayCastCallbackFunction {
  rayBack(fixture: B2Fixture, point: B2Vec2, normal: B2Vec2, fraction: number): number;
}

export class B2RayCastCallbackProxy implements B2RayCastCallback {
  callback: B2RayCastCallbackFunction;
  constructor(callback: B2RayCastCallbackFunction) {
    this.callback = callback;
  }
  reportFixture(fixture: B2Fixture, point: B2Vec2, normal: B2Vec2, fraction: number): number {
    return this.callback.rayBack(fixture, point, normal, fraction);
  }
}
export { B2RayCastCallbackFunction, B2RayCastCallback, B2QueryCallbackFunction, B2QueryCallback, B2ContactListener, B2DestructionListener };
