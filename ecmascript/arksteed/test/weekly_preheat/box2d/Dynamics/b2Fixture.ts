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
import { B2Shape } from '../Collision/Shapes/b2Shape';
import { B2RayCastOutput, B2RayCastInput } from '../Collision/b2Collision';
import { B2AaBb } from '../Collision/b2Collision';
import { B2ShapeType, B2MassData } from '../Collision/Shapes/b2Shape';
import { B2Vec2, B2Transform } from '../Common/b2Math';
import { subtract } from '../Common/b2Math';
import { B2BroadPhase } from '../Collision/b2BroadPhase';
import { B2Body } from './b2Body';

export class B2Filter {
  constructor() {
    this.categoryBits = 0x0001;
    this.maskBits = 0xffff;
    this.groupIndex = 0;
  }
  public categoryBits: number;
  public maskBits: number;
  public groupIndex: number;
}
export class B2FixtureDef {
  constructor() {
    this.shape = null;
    this.userData = null;
    this.friction = 0.2;
    this.restitution = 0.0;
    this.density = 0.0;
    this.isSensor = false;
    this.filter = new B2Filter();
  }
  shape: B2Shape | null;
  userData?: object | null;
  friction: number;
  restitution: number;
  density: number;
  isSensor: boolean;
  filter: B2Filter;
}

export class B2FixtureProxy {
  constructor(fixture: B2Fixture) {
    this.fixture = fixture;
  }
  aabb = new B2AaBb();
  fixture: B2Fixture;
  childIndex = 0;
  proxyId = 0;
}

export class B2Fixture {
  mDensity: number = 0.0;
  mNext: B2Fixture | null = null;
  mBody: B2Body;
  mShape: B2Shape | null = null;
  mFriction: number = 0.0;
  mRestitution: number = 0.0;
  mProxies: B2FixtureProxy[] = [];
  mFilter = new B2Filter();
  mIsSensor: boolean = false;
  mUserData?: object | null = null;

  constructor(body: B2Body, def: B2FixtureDef) {
    this.mUserData = def.userData;
    this.mFriction = def.friction;
    this.mRestitution = def.restitution;
    this.mBody = body;
    this.mNext = null;
    this.mFilter = def.filter;
    this.mIsSensor = def.isSensor;
    this.mShape = def.shape!.clone();
    this.mProxies = [];
    this.mDensity = def.density;
  }

  get mProxyCount(): number {
    return this.mProxies.length;
  }

  get fixtureType(): B2ShapeType {
    return this.mShape!.type();
  }

  get shape(): B2Shape {
    return this.mShape!;
  }

  get isSensor(): boolean {
    return this.mIsSensor;
  }

  set isSensor(newValue) {
    this.setSensor(newValue);
  }

  setSensor(sensor: boolean): void {
    if (sensor !== this.mIsSensor) {
      this.mBody.setAwake(true);
      this.mIsSensor = sensor;
    }
  }

  get filterData(): B2Filter {
    return this.mFilter;
  }

  set filterData(newValue) {
    this.setFilterData(newValue);
  }

  setFilterData(filter: B2Filter): void {
    this.mFilter = filter;
    this.refilter();
  }

  refilter(): void {
    let edge = this.mBody.getContactList();
    while (edge !== null) {
      let contact = edge!.contact;
      let fixtureA = contact.fixtureA;
      let fixtureB = contact.fixtureB;
      if (fixtureA === this || fixtureB === this) {
        contact.flagForFiltering();
      }
      edge = edge!.next;
    }
    let world = this.mBody.world;
    if (world === null) {
      return;
    }
    let broadPhase = world.mContactManager.mBroadPhase;
    for (let i = 0; i < this.mProxyCount; i++) {
      broadPhase.touchProxy(this.mProxies[i].proxyId);
    }
  }

  get body(): B2Body {
    return this.mBody;
  }

  getNext(): B2Fixture | undefined {
    if (this.mNext !== null) {
      return this.mNext;
    }
    return undefined;
  }

  get userData(): object | undefined {
    if (this.mUserData !== null) {
      return this.mUserData;
    }
    return undefined;
  }

  set userData(newValue) {
    this.setUserData(newValue);
  }

  setUserData(data?: object): void {
    this.mUserData = data;
  }

  testPoint(p: B2Vec2): boolean {
    return this.mShape!.testPoint(this.mBody.transform, p);
  }

  rayCast(output: B2RayCastOutput, input: B2RayCastInput, childIndex: number): boolean {
    return this.mShape!.rayCast(output, input, this.mBody.transform, childIndex);
  }

  get massData(): B2MassData {
    return this.mShape!.computeMass(this.mDensity);
  }

  get density(): number {
    return this.mDensity;
  }

  set density(newValue) {
    this.setDensity(newValue);
  }

  setDensity(density: number): void {
    this.mDensity = density;
  }

  get friction(): number {
    return this.mFriction;
  }

  set friction(newValue) {
    this.setFriction(newValue);
  }

  setFriction(friction: number): void {
    this.mFriction = friction;
  }

  get restitution(): number {
    return this.mRestitution;
  }

  set restitution(newValue) {
    this.setRestitution(newValue);
  }

  setRestitution(restitution: number): void {
    this.mRestitution = restitution;
  }

  getAaBb(childIndex: number): B2AaBb {
    return this.mProxies[childIndex].aabb;
  }

  destroy(): void {
    this.mProxies.splice(0, this.mProxies.length);
    this.mShape = null;
  }

  createProxies(broadPhase: B2BroadPhase, xf: B2Transform): void {
    if (this.mShape !== null) {
      let proxyCount = this.mShape.childCount();
      for (let i = 0; i < proxyCount; i++) {
        let proxy = new B2FixtureProxy(this);
        this.mShape!.computeAaBb(proxy.aabb, xf, i);
        proxy.childIndex = i;
        proxy.proxyId = broadPhase.createProxy(proxy.aabb, proxy);
        this.mProxies.push(proxy);
      }
    }
  }

  destroyProxies(broadPhase: B2BroadPhase): void {
    for (let i = 0; i < this.mProxyCount; i++) {
      let proxy = this.mProxies[i];
      broadPhase.destroyProxy(proxy.proxyId);
    }
    this.mProxies.splice(0, this.mProxies.length);
  }

  synchronize(broadPhase: B2BroadPhase, transform1: B2Transform, transform2: B2Transform): void {
    if (this.mProxyCount === 0) {
      return;
    }
    for (let i = 0; i < this.mProxyCount; i++) {
      let proxy = this.mProxies[i];
      let aabb1 = new B2AaBb();
      let aabb2 = new B2AaBb();
      this.mShape!.computeAaBb(aabb1, transform1, proxy.childIndex);
      this.mShape!.computeAaBb(aabb2, transform2, proxy.childIndex);
      proxy.aabb.combine(aabb1, aabb2);
      let displacement = subtract(transform2.p, transform1.p);
      broadPhase.moveProxy(proxy.proxyId, proxy.aabb, displacement);
    }
  }
}
