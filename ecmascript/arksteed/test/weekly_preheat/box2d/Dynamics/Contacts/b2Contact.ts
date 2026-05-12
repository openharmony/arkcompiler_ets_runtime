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
import { B2CircleShape, B2EdgeShape, B2PolygonShape } from '../../Collision/Shapes/b2Shape';
import { B2ShapeType } from '../../Collision/Shapes/b2Shape';
import { B2Transform } from '../../Common/b2Math';
import { B2ContactFeature } from '../../Collision/b2Collision';
import { B2WorldManifold, B2Manifold, b2TestOverlap } from '../../Collision/b2Collision';
import { B2ContactListener } from '../b2WorldCallbacks';
import { B2Body } from '../b2Body';
import { B2Fixture } from '../b2Fixture';
import { b2CollidePolygons } from '../../Collision/b2CollidePolygon';
import { b2CollideCircles, b2CollidePolygonAndCircle } from '../../Collision/b2CollideCircles';
import { b2CollideEdgeAndCircle, b2CollideEdgeAndPolygon } from '../../Collision/b2CollideEdge';

function b2MixFriction(friction1: number, friction2: number): number {
  return Math.sqrt(friction1 * friction2);
}

function b2MixRestitution(restitution1: number, restitution2: number): number {
  return restitution1 > restitution2 ? restitution1 : restitution2;
}

type B2ContactCreateFcn = (fixtureA: B2Fixture, indexA: number, fixtureB: B2Fixture, indexB: number) => B2Contact;
type B2ContactDestroyFcn = (contact: B2Contact) => void;

export class B2ContactRegister {
  createFcn: B2ContactCreateFcn | null = null;
  destroyFcn: B2ContactDestroyFcn | null = null;
  primary: boolean = false;
}

export class B2ContactRegisters {
  rows: B2ShapeType;
  columns: B2ShapeType;
  grid: B2ContactRegister[] = [];
  constructor(rows: B2ShapeType, columns: B2ShapeType) {
    this.rows = rows;
    this.columns = columns;
    for (let i = 0; i < rows * columns; i++) {
      this.grid.push(new B2ContactRegister());
    }
  }
  
  getItem(row: B2ShapeType, column: B2ShapeType): B2ContactRegister {
    return this.grid[row * this.columns + column];
  }
  
  setItem(row: B2ShapeType, column: B2ShapeType, value: B2ContactRegister): void {
    this.grid[row * this.columns + column] = value;
  }
}

export class B2ContactEdge {
  other: B2Body | null = null;
  contact: B2Contact;
  prev: B2ContactEdge | null = null;
  next: B2ContactEdge | null = null;
  constructor(contact: B2Contact) {
    this.contact = contact;
  }
}

export class B2Contact {
  mFlags: number = 0;
  mPrev: B2Contact | null = null;
  mNext: B2Contact | null = null;
  mNodeA: B2ContactEdge;
  mNodeB: B2ContactEdge;
  mFixtureA: B2Fixture;
  mFixtureB: B2Fixture;
  mIndexA: number = 0;
  mIndexB: number = 0;
  mManifold = new B2Manifold();
  mToiCount: number = 0;
  mToi: number = 0;
  mFriction: number = 0;
  mRestitution: number = 0;
  mTangentSpeed: number = 0;
  get manifold(): B2Manifold {
    return this.mManifold;
  }

  get worldManifold(): B2WorldManifold {
    let bodyA = this.mFixtureA.body;
    let bodyB = this.mFixtureB.body;
    let shapeA = this.mFixtureA.shape;
    let shapeB = this.mFixtureB.shape;
    let worldManifold = new B2WorldManifold();
    worldManifold.initialize(this.mManifold, bodyA.transform, shapeA!.mRadius, bodyB.transform, shapeB!.mRadius);
    return worldManifold;
  }

  get isTouching(): boolean {
    return (this.mFlags & FlagContacts.TOUCHINGFLAG) === FlagContacts.TOUCHINGFLAG;
  }

  setEnabled(flag: boolean): void {
    if (flag) {
      this.mFlags |= FlagContacts.ENABLEDFLAG;
    } else {
      this.mFlags &= ~FlagContacts.ENABLEDFLAG;
    }
  }

  get isEnabled(): boolean {
    return (this.mFlags & FlagContacts.ENABLEDFLAG) === FlagContacts.ENABLEDFLAG;
  }
  
  getNext(): B2Contact | null {
    return this.mNext;
  }

  get fixtureA(): B2Fixture {
    return this.mFixtureA;
  }

  get childIndexA(): number {
    return this.mIndexA;
  }

  get fixtureB(): B2Fixture {
    return this.mFixtureB;
  }

  get childIndexB(): number {
    return this.mIndexB;
  }

  get friction(): number {
    return this.mFriction;
  }

  set setFriction(friction: number) {
    this.mFriction = friction;
  }

  resetFriction(): void {
    this.mFriction = b2MixFriction(this.mFixtureA.mFriction, this.mFixtureB.mFriction);
  }

  get restitution(): number {
    return this.mRestitution;
  }

  set setRestitution(restitution: number) {
    this.mRestitution = restitution;
  }

  resetRestitution(): void {
    this.mRestitution = b2MixRestitution(this.mFixtureA.mRestitution, this.mFixtureB.mRestitution);
  }

  setTangentSpeed(speed: number): void {
    this.mTangentSpeed = speed;
  }

  get tangentSpeed(): number {
    return this.mTangentSpeed;
  }

  evaluate(manifold: B2Manifold, xfA: B2Transform, xfB: B2Transform): void {
    throw new Error('must override');
  }

  flagForFiltering(): void {
    throw new Error('must override');
  }

  static addType(createFcn: B2ContactCreateFcn, destroyFcn: B2ContactDestroyFcn, type1: B2ShapeType, type2: B2ShapeType): void {
    StaticVars.sRegisters.getItem(type1, type2).createFcn = createFcn;
    StaticVars.sRegisters.getItem(type1, type2).destroyFcn = destroyFcn;
    StaticVars.sRegisters.getItem(type1, type2).primary = true;
    if (type1 !== type2) {
      StaticVars.sRegisters.getItem(type2, type1).createFcn = createFcn;
      StaticVars.sRegisters.getItem(type2, type1).destroyFcn = destroyFcn;
      StaticVars.sRegisters.getItem(type2, type1).primary = false;
    }
  }

  static initializeRegisters(): void {
    B2Contact.addType(B2PolygonContact.create, B2PolygonContact.destroy, B2ShapeType.POLYGON, B2ShapeType.POLYGON);
  }

  static create(fixtureA: B2Fixture, indexA: number, fixtureB: B2Fixture, indexB: number): B2Contact | null {
    if (StaticVars.sInitialized === false) {
      B2Contact.initializeRegisters();
      StaticVars.sInitialized = true;
    }
    let type1 = fixtureA!.fixtureType;
    let type2 = fixtureB!.fixtureType;
    let createFcn = StaticVars.sRegisters.getItem(type1!, type2!).createFcn;
    if (createFcn !== null) {
      if (StaticVars.sRegisters.getItem(type1!, type2!).primary) {
        return createFcn(fixtureA, indexA, fixtureB, indexB);
      } else {
        return createFcn(fixtureB, indexB, fixtureA, indexA);
      }
    } else {
      return null;
    }
  }

  static destroy(contact: B2Contact): void {
    let fixtureA = contact.mFixtureA;
    let fixtureB = contact.mFixtureB;
    if (contact.mManifold.pointCount > 0 && fixtureA?.isSensor === false && fixtureB?.isSensor === false) {
      fixtureA?.body.setAwake(true);
      fixtureB?.body.setAwake(true);
    }
    let typeA = fixtureA?.fixtureType;
    let typeB = fixtureB?.fixtureType;
    let destroyFcn = StaticVars.sRegisters.getItem(typeA!, typeB!).destroyFcn;
    destroyFcn!(contact);
  }

  constructor(fixtureA: B2Fixture, indexA: number, fixtureB: B2Fixture, indexB: number) {
    this.mFlags = FlagContacts.ENABLEDFLAG;
    this.mFixtureA = fixtureA;
    this.mFixtureB = fixtureB;
    this.mIndexA = indexA;
    this.mIndexB = indexB;
    this.mManifold.points.splice(0, this.mManifold.points.length);
    this.mPrev = null;
    this.mNext = null;
    this.mNodeA = new B2ContactEdge(this);
    this.mNodeA.prev = null;
    this.mNodeA.next = null;
    this.mNodeA.other = null;
    this.mNodeB = new B2ContactEdge(this);
    this.mNodeB.prev = null;
    this.mNodeB.next = null;
    this.mNodeB.other = null;
    this.mToiCount = 0;
    this.mFriction = b2MixFriction(this.mFixtureA.mFriction, this.mFixtureB.mFriction);
    this.mRestitution = b2MixRestitution(this.mFixtureA.mRestitution, this.mFixtureB.mRestitution);
    this.mTangentSpeed = 0.0;
  }

  update(listener: B2ContactListener): void {
    let oldManifold = new B2Manifold(this.mManifold);
    this.mFlags |= FlagContacts.ENABLEDFLAG;
    let touching = false;
    let wasTouching = (this.mFlags & FlagContacts.TOUCHINGFLAG) === FlagContacts.TOUCHINGFLAG;
    let sensorA = this.mFixtureA.isSensor;
    let sensorB = this.mFixtureB.isSensor;
    let sensor = sensorA || sensorB;
    let bodyA = this.mFixtureA.body;
    let bodyB = this.mFixtureB.body;
    let xfA = bodyA.transform;
    let xfB = bodyB.transform;
    if (sensor) {
      let shapeA = this.mFixtureA.shape;
      let shapeB = this.mFixtureB.shape;
      touching = b2TestOverlap(shapeA!, shapeB!, xfA, xfB);
      this.mManifold.points.splice(0, this.mManifold.points.length);
    } else {
      this.evaluate(this.mManifold, xfA, xfB);
      touching = this.mManifold.pointCount > 0;
      for (let i = 0; i < this.mManifold.pointCount; i++) {
        let mp2 = this.mManifold.points[i];
        mp2.normalImpulse = 0.0;
        mp2.tangentImpulse = 0.0;
        let id2 = mp2.id;
        for (let j = 0; j < oldManifold.pointCount; j++) {
          let mp1 = oldManifold.points[j];
          if (equalEqual(mp1.id, id2)) {
            mp2.normalImpulse = mp1.normalImpulse;
            mp2.tangentImpulse = mp1.tangentImpulse;
            break;
          }
        }
      }
      if (touching !== wasTouching) {
        bodyA.setAwake(true);
        bodyB.setAwake(true);
      }
    }
  }
}

export class StaticVars {
  static sRegisters = new B2ContactRegisters(B2ShapeType.TYPECOUNT, B2ShapeType.TYPECOUNT);
  static sInitialized: boolean = false;
}

const hex0001: number = 0x0001;
const hex0002: number = 0x0002;
const hex0004: number = 0x0004;
const hex0008: number = 0x0008;
const hex0010: number = 0x0010;
const hex0020: number = 0x0020;

enum FlagContacts {
  ISLANDFLAG = hex0001,
  TOUCHINGFLAG = hex0002,
  ENABLEDFLAG = hex0004,
  FILTERFLAG = hex0008,
  BULLETHITFLAG = hex0010,
  TOIFLAG = hex0020
}

export class B2PolygonContact extends B2Contact {
  static create(fixtureA: B2Fixture, indexA: number, fixtureB: B2Fixture, indexB: number): B2Contact {
    return new B2PolygonContact(fixtureA, fixtureB);
  }

  static destroy(contact: B2Contact): void {}

  constructor(fixtureA: B2Fixture, fixtureB: B2Fixture) {
    super(fixtureA, 0, fixtureB, 0);
  }

  evaluate(manifold: B2Manifold, xfA: B2Transform, xfB: B2Transform): void {
    b2CollidePolygons(manifold, this.mFixtureA.shape as B2PolygonShape, xfA, this.mFixtureB.shape as B2PolygonShape, xfB);
  }
}

export class B2CircleContact extends B2Contact {
  static create(fixtureA: B2Fixture, indexA: number, fixtureB: B2Fixture, indexB: number): B2Contact {
    return new B2CircleContact(fixtureA, fixtureB);
  }
  
  static destroy(contact: B2Contact): void {}
  
  constructor(fixtureA: B2Fixture, fixtureB: B2Fixture) {
    super(fixtureA, 0, fixtureB, 0);
  }
  
  evaluate(manifold: B2Manifold, xfA: B2Transform, xfB: B2Transform): void {
    b2CollideCircles(manifold, this.mFixtureA.shape as B2CircleShape, xfA, this.mFixtureB.shape as B2CircleShape, xfB);
  }
}

export class B2EdgeAndCircleContact extends B2Contact {
  static create(fixtureA: B2Fixture, indexA: number, fixtureB: B2Fixture, indexB: number): B2Contact {
    return new B2EdgeAndCircleContact(fixtureA, fixtureB);
  }

  static destroy(contact: B2Contact): void {}

  constructor(fixtureA: B2Fixture, fixtureB: B2Fixture) {
    super(fixtureA, 0, fixtureB, 0);
  }

  evaluate(manifold: B2Manifold, xfA: B2Transform, xfB: B2Transform): void {
    b2CollideEdgeAndCircle(manifold, this.mFixtureA.shape as B2EdgeShape, xfA, this.mFixtureB.shape as B2CircleShape, xfB);
  }
}

export class B2EdgeAndPolygonContact extends B2Contact {
  static create(fixtureA: B2Fixture, indexA: number, fixtureB: B2Fixture, indexB: number): B2Contact {
    return new B2EdgeAndPolygonContact(fixtureA, fixtureB);
  }

  static destroy(contact: B2Contact): void {}

  constructor(fixtureA: B2Fixture, fixtureB: B2Fixture) {
    super(fixtureA, 0, fixtureB, 0);
  }

  evaluate(manifold: B2Manifold, xfA: B2Transform, xfB: B2Transform): void {
    b2CollideEdgeAndPolygon(manifold, this.mFixtureA.shape as B2EdgeShape, xfA, this.mFixtureB.shape as B2PolygonShape, xfB);
  }
}

export class B2PolygonAndCircleContact extends B2Contact {
  static create(fixtureA: B2Fixture, indexA: number, fixtureB: B2Fixture, indexB: number): B2Contact {
    return new B2PolygonAndCircleContact(fixtureA, fixtureB);
  }

  static destroy(contact: B2Contact): void {}

  constructor(fixtureA: B2Fixture, fixtureB: B2Fixture) {
    super(fixtureA, 0, fixtureB, 0);
  }

  evaluate(manifold: B2Manifold, xfA: B2Transform, xfB: B2Transform): void {
    b2CollidePolygonAndCircle(manifold, this.mFixtureA.shape as B2PolygonShape, xfA, this.mFixtureB.shape as B2CircleShape, xfB);
  }
}

export function equalEqual(lhs: B2ContactFeature, rhs: B2ContactFeature): Boolean {
  return lhs.indexA === rhs.indexA && lhs.indexB === rhs.indexB && lhs.typeA === rhs.typeA && lhs.typeB === rhs.typeB;
}

export { FlagContacts };
