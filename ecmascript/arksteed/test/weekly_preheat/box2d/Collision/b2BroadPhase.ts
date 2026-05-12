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
import { B2RayCastInput, B2AaBb } from './b2Collision';
import { b2TestOverlap2 } from './b2Collision';
import { B2QueryWrapper, B2BroadPhaseWrapper, B2RayCastWrapper } from '../Common/b2Wrappers';
import { B2FixtureProxy } from '../Dynamics/b2Fixture';
import { B2DynamicTree } from './b2DynamicTree';
export class B2Pair {
  public proxyIdA: number = -1;
  public proxyIdB: number = -1;
  constructor(proxyIdA: number = -1, proxyIdB: number = -1) {
    this.proxyIdA = proxyIdA;
    this.proxyIdB = proxyIdB;
  }
}

export class B2BroadPhase implements B2QueryWrapper {
  public static nullProxy = -1;
  public mTree = new B2DynamicTree<B2FixtureProxy>();
  public mProxyCount: number = 0;
  public mMoveBuffer: number[] = [];
  public mPairBuffer: B2Pair[] = [];
  public mQueryProxyId: number = 0;

  constructor() {
    this.mProxyCount = 0;
    this.mPairBuffer = [];
    this.mMoveBuffer = [];
  }

  public createProxy(aabb: B2AaBb, userData: B2FixtureProxy): number {
    let proxyId = this.mTree.createProxy(aabb, userData);
    this.mProxyCount += 1;
    this.bufferMove(proxyId);
    return proxyId;
  }

  public destroyProxy(proxyId: number): void {
    this.unBufferMove(proxyId);
    this.mProxyCount -= 1;
    this.mTree.destroyProxy(proxyId);
  }

  public moveProxy(proxyId: number, aabb: B2AaBb, displacement: B2Vec2): void {
    let buffer = this.mTree.moveProxy(proxyId, aabb, displacement);
    if (buffer) {
      this.bufferMove(proxyId);
    }
  }

  public touchProxy(proxyId: number): void {
    this.bufferMove(proxyId);
  }

  public getFatAaBb(proxyId: number): B2AaBb {
    return this.mTree.getFatAaBb(proxyId);
  }

  public getUserData(proxyId: number): B2FixtureProxy | null {
    return this.mTree.getUserData(proxyId);
  }

  public testOverlap(proxyIdA: number, proxyIdB: number): boolean {
    let aabbA = this.mTree.getFatAaBb(proxyIdA);
    let aabbB = this.mTree.getFatAaBb(proxyIdB);
    return b2TestOverlap2(aabbA, aabbB);
  }

  public getProxyCount(): number {
    return this.mProxyCount;
  }

  public updatePairs<T extends B2BroadPhaseWrapper>(callback: T): void {
    this.mPairBuffer.splice(0, this.mPairBuffer.length);
    for (let i = 0; i < this.mMoveBuffer.length; i++) {
      this.mQueryProxyId = this.mMoveBuffer[i];
      if (this.mQueryProxyId === B2BroadPhase.nullProxy) {
        continue;
      }
      let fatAaBb = this.mTree.getFatAaBb(this.mQueryProxyId);
      this.mTree.query(this, fatAaBb);
    }
    this.mMoveBuffer.splice(0, this.mMoveBuffer.length);
    this.mPairBuffer.sort((a, b) => {
      if (a.proxyIdA === b.proxyIdA) {
        return a.proxyIdB - b.proxyIdB;
      } else {
        return a.proxyIdA - b.proxyIdA;
      }
    });
    let i = 0;
    while (i < this.mPairBuffer.length) {
      let primaryPair = this.mPairBuffer[i];
      let userDataA = this.mTree.getUserData(primaryPair.proxyIdA)!;
      let userDataB = this.mTree.getUserData(primaryPair.proxyIdB)!;
      callback.addPair(userDataA, userDataB);
      i += 1;
      while (i < this.mPairBuffer.length) {
        let pair = this.mPairBuffer[i];
        if (pair.proxyIdA !== primaryPair.proxyIdA || pair.proxyIdB !== primaryPair.proxyIdB) {
          break;
        }
        i += 1;
      }
    }
  }
  
  public query<T extends B2QueryWrapper>(callback: T, aabb: B2AaBb): void {
    this.mTree.query(callback, aabb);
  }
  
  public rayCast<T extends B2RayCastWrapper>(callback: T, input: B2RayCastInput): void {
    this.mTree.rayCast(callback, input);
  }
  
  public getTreeHeight(): number {
    return this.mTree.getHeight();
  }
  
  public getTreeBalance(): number {
    return this.mTree.getMaxBalance();
  }
  
  public getTreeQuality(): number {
    return this.mTree.getAreaRatio();
  }
  
  public shiftOrigin(newOrigin: B2Vec2): void {
    this.mTree.shiftOrigin(newOrigin);
  }
  
  bufferMove(proxyId: number): void {
    this.mMoveBuffer.push(proxyId);
  }
  
  unBufferMove(proxyId: number): void {
    for (let i = 0; i < this.mMoveBuffer.length; i++) {
      if (this.mMoveBuffer[i] === proxyId) {
        this.mMoveBuffer[i] = B2BroadPhase.nullProxy;
      }
    }
  }
  
  public queryCallback(proxyId: number): boolean {
    if (proxyId === this.mQueryProxyId) {
      return true;
    }
    let pair = new B2Pair(Math.min(proxyId, this.mQueryProxyId), Math.max(proxyId, this.mQueryProxyId));
    this.mPairBuffer.push(pair);
    return true;
  }
}
