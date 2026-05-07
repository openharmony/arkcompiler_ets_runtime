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
import { B2AaBb, B2RayCastInput, b2TestOverlap2 } from './b2Collision';
import { B2Vec2, subtract, add, multM, b2Cross21, b2Abs2, b2Min, b2Max, b2Dot22, subtractEqual } from '../Common/b2Math';
import { B2QueryWrapper, B2RayCastWrapper } from '../Common/b2Wrappers';
import { b2AaBbExtension, b2AaBbMultiplier, b2MaxFloat, indexTwo } from '../Common/b2Settings';
import { float2 } from '../Common/b2Settings';

let b2NullNode = -1;
export class B2TreeNode<T> {
  public aabb = new B2AaBb();
  public userData: T | null = null;
  parentOrNext: number = b2NullNode;
  child1: number = b2NullNode;
  child2: number = b2NullNode;
  height: number = -1;
  isLeaf(): boolean {
    return this.child1 === b2NullNode;
  }
}

export class B2DynamicTree<T> {
  mRoot: number;
  mNodes: B2TreeNode<T>[];
  mNodeCount: number;
  mNodeCapacity: number;
  mFreeList: number;
  mInsertionCount: number;
  constructor() {
    this.mRoot = b2NullNode;
    this.mNodeCapacity = 16;
    this.mNodeCount = 0;
    this.mNodes = new Array<B2TreeNode<T>>();
    for (let i = 0; i < this.mNodeCapacity - 1; i++) {
      this.mNodes.push(new B2TreeNode());
      this.mNodes[this.mNodes.length - 1].parentOrNext = i + 1;
      this.mNodes[this.mNodes.length - 1].height = -1;
    }
    this.mNodes.push(new B2TreeNode());
    this.mNodes[this.mNodes.length - 1].parentOrNext = b2NullNode;
    this.mNodes[this.mNodes.length - 1].height = -1;
    this.mFreeList = 0;
    this.mInsertionCount = 0;
  }

  public createProxy(aabb: B2AaBb, userData: T): number {
    let proxyId = this.allocateNode();
    let r = new B2Vec2(b2AaBbExtension, b2AaBbExtension);
    this.mNodes[proxyId].aabb.lowerBound = subtract(aabb.lowerBound, r);
    this.mNodes[proxyId].aabb.upperBound = add(aabb.upperBound, r);
    this.mNodes[proxyId].userData = userData;
    this.mNodes[proxyId].height = 0;
    this.insertLeaf(proxyId);
    return proxyId;
  }

  public destroyProxy(proxyId: number): void {
    this.removeLeaf(proxyId);
    this.freeNode(proxyId);
  }

  public moveProxy(proxyId: number, aabb: B2AaBb, displacement: B2Vec2): boolean {
    if (this.mNodes[proxyId].aabb.contains(aabb)) {
      return false;
    }
    this.removeLeaf(proxyId);
    let b = aabb;
    let r = new B2Vec2(b2AaBbExtension, b2AaBbExtension);
    b.lowerBound = subtract(b.lowerBound, r);
    b.upperBound = add(b.upperBound, r);
    let d = multM(displacement, b2AaBbMultiplier);
    if (d.x < 0.0) {
      b.lowerBound.x += d.x;
    } else {
      b.upperBound.x += d.x;
    }
    if (d.y < 0.0) {
      b.lowerBound.y += d.y;
    } else {
      b.upperBound.y += d.y;
    }
    this.mNodes[proxyId].aabb = b;
    this.insertLeaf(proxyId);
    return true;
  }

  public getUserData(proxyId: number): T | null {
    return this.mNodes[proxyId].userData;
  }

  public getFatAaBb(proxyId: number): B2AaBb {
    return this.mNodes[proxyId].aabb;
  }
  public query<T extends B2QueryWrapper>(callback: T, aabb: B2AaBb): void {
    let stack: number[] = [];
    stack.push(this.mRoot);
    while (stack.length > 0) {
      let nodeId = stack.pop();
      if (nodeId === b2NullNode) {
        continue;
      }
      let node = this.mNodes[nodeId!];
      if (b2TestOverlap2(node.aabb, aabb)) {
        if (node.isLeaf()) {
          let proceed = callback.queryCallback(nodeId!);
          if (proceed === false) {
            return;
          }
        } else {
          stack.push(node.child1);
          stack.push(node.child2);
        }
      }
    }
  }

  rayCast<T extends B2RayCastWrapper>(callback: T, input: B2RayCastInput): void {
    let p1 = input.p1;
    let p2 = input.p2;
    let r = subtract(p2, p1);
    r.normalize();
    let v = b2Cross21(1.0, r);
    let absv = b2Abs2(v);
    let maxFraction = input.maxFraction;
    let segmentAaBb = new B2AaBb();
    let t = add(p1, multM(subtract(p2, p1), maxFraction));
    segmentAaBb.lowerBound = b2Min(p1, t);
    segmentAaBb.upperBound = b2Max(p1, t);
    let stack: number[] = [];
    stack.push(this.mRoot);
    while (stack.length > 0) {
      let nodeId = stack.pop();
      if (nodeId === b2NullNode) {
        continue;
      }
      let node = this.mNodes[nodeId!];
      if (b2TestOverlap2(node.aabb, segmentAaBb) === false) {
        continue;
      }
      let c = node.aabb.center;
      let h = node.aabb.extents;
      let separation = Math.abs(b2Dot22(v, subtract(p1, c))) - b2Dot22(absv, h);
      if (separation > 0.0) {
        continue;
      }
      if (node.isLeaf()) {
        let subInput = new B2RayCastInput();
        subInput.p1 = input.p1;
        subInput.p2 = input.p2;
        subInput.maxFraction = maxFraction;
        let value = callback.rayCastCallback(subInput, nodeId!);
        if (value === 0.0) {
          return;
        }
        if (value > 0.0) {
          maxFraction = value;
          let t = add(p1, multM(subtract(p2, p1), maxFraction));
          segmentAaBb.lowerBound = b2Min(p1, t);
          segmentAaBb.upperBound = b2Max(p1, t);
        }
      } else {
        stack.push(node.child1);
        stack.push(node.child2);
      }
    }
  }

  public validate(): void {
    this.validateStructure(this.mRoot);
    this.validateMetrics(this.mRoot);
    let freeIndex = this.mFreeList;
    while (freeIndex !== b2NullNode) {
      freeIndex = this.mNodes[freeIndex].parentOrNext;
    }
  }

  public getHeight(): number {
    if (this.mRoot === b2NullNode) {
      return 0;
    }
    return this.mNodes[this.mRoot].height;
  }

  public getMaxBalance(): number {
    let maxBalance = 0;
    for (let i = 0; i < this.mNodes.length; i++) {
      let node = this.mNodes[i];
      if (node.height <= 1) {
        continue;
      }
      let child1 = node.child1;
      let child2 = node.child2;
      let balance = Math.abs(this.mNodes[child2].height - this.mNodes[child1].height);
      maxBalance = Math.max(maxBalance, balance);
    }
    return maxBalance;
  }

  public getAreaRatio(): number {
    if (this.mRoot === b2NullNode) {
      return 0.0;
    }
    let root = this.mNodes[this.mRoot];
    let rootArea = root.aabb.perimeter;
    let totalArea: number = 0.0;
    for (let i = 0; i < this.mNodes.length; i++) {
      let node = this.mNodes[i];
      if (node.height < 0) {
        continue;
      }
      totalArea += node.aabb.perimeter;
    }
    return totalArea / rootArea;
  }
  
  public rebuildBottomUp(): void {
    let nodes: number[] = [];
    for (let i = 0; i < this.mNodes.length; i++) {
      nodes.push(0);
    }
    let count = 0;
    for (let i = 0; i < this.mNodes.length; i++) {
      if (this.mNodes[i].height < 0) {
        continue;
      }
      if (this.mNodes[i].isLeaf()) {
        this.mNodes[i].parentOrNext = b2NullNode;
        nodes[count] = i;
        count += 1;
      } else {
        this.freeNode(i);
      }
    }
    while (count > 1) {
      let minCost = b2MaxFloat;
      let iMin = -1;
      let jMin = -1;
      for (let i = 0; i < count; i++) {
        let aabbi = this.mNodes[nodes[i]].aabb;
        for (let j = i + 1; j < count; j++) {
          let aabbj = this.mNodes[nodes[j]].aabb;
          let b = new B2AaBb();
          b.combine(aabbi, aabbj);
          let cost = b.perimeter;
          if (cost < minCost) {
            iMin = i;
            jMin = j;
            minCost = cost;
          }
        }
      }
      let index1 = nodes[iMin];
      let index2 = nodes[jMin];
      let child1 = this.mNodes[index1];
      let child2 = this.mNodes[index2];
      let parentIndex = this.allocateNode();
      let parent = this.mNodes[parentIndex];
      parent.child1 = index1;
      parent.child2 = index2;
      parent.height = 1 + Math.max(child1.height, child2.height);
      parent.aabb.combine(child1.aabb, child2.aabb);
      parent.parentOrNext = b2NullNode;
      child1.parentOrNext = parentIndex;
      child2.parentOrNext = parentIndex;
      nodes[jMin] = nodes[count - 1];
      nodes[iMin] = parentIndex;
      count -= 1;
    }
    this.mRoot = nodes[0];
    this.validate();
  }

  public shiftOrigin(newOrigin: B2Vec2): void {
    for (let i = 0; i < this.mNodes.length; i++) {
      subtractEqual(this.mNodes[i].aabb.lowerBound, newOrigin);
      subtractEqual(this.mNodes[i].aabb.upperBound, newOrigin);
    }
  }

  allocateNode(): number {
    if (this.mFreeList === b2NullNode) {
      let node = new B2TreeNode<T>();
      node.parentOrNext = b2NullNode;
      node.height = -1;
      this.mNodes.push(node);
      this.mFreeList = this.mNodes.length - 1;
    }
    let nodeId = this.mFreeList;
    this.mFreeList = this.mNodes[nodeId].parentOrNext;
    this.mNodes[nodeId].parentOrNext = b2NullNode;
    this.mNodes[nodeId].child1 = b2NullNode;
    this.mNodes[nodeId].child2 = b2NullNode;
    this.mNodes[nodeId].height = 0;
    this.mNodes[nodeId].userData = null;
    return nodeId;
  }

  freeNode(nodeId: number): void {
    this.mNodes[nodeId].parentOrNext = this.mFreeList;
    this.mNodes[nodeId].height = -1;
    this.mFreeList = nodeId;
  }

  insertLeaf(leaf: number): void {
    this.mInsertionCount += 1;
    if (this.mRoot === b2NullNode) {
      this.mRoot = leaf;
      this.mNodes[this.mRoot].parentOrNext = b2NullNode;
      return;
    }
    let leafAaBb = this.mNodes[leaf].aabb;
    let index = this.mRoot;
    while (this.mNodes[index].isLeaf() === false) {
      let child1 = this.mNodes[index].child1;
      let child2 = this.mNodes[index].child2;
      let area = this.mNodes[index].aabb.perimeter;
      let combinedAaBb = new B2AaBb();
      combinedAaBb.combine(this.mNodes[index].aabb, leafAaBb);
      let combinedArea = combinedAaBb.perimeter;
      let cost = float2 * combinedArea;
      let inheritanceCost = float2 * (combinedArea - area);
      let cost1: number;
      if (this.mNodes[child1].isLeaf()) {
        let aabb = new B2AaBb();
        aabb.combine(leafAaBb, this.mNodes[child1].aabb);
        cost1 = aabb.perimeter + inheritanceCost;
      } else {
        let aabb = new B2AaBb();
        aabb.combine(leafAaBb, this.mNodes[child1].aabb);
        let oldArea = this.mNodes[child1].aabb.perimeter;
        let newArea = aabb.perimeter;
        cost1 = newArea - oldArea + inheritanceCost;
      }
      let cost2: number;
      if (this.mNodes[child2].isLeaf()) {
        let aabb = new B2AaBb();
        aabb.combine(leafAaBb, this.mNodes[child2].aabb);
        cost2 = aabb.perimeter + inheritanceCost;
      } else {
        let aabb = new B2AaBb();
        aabb.combine(leafAaBb, this.mNodes[child2].aabb);
        let oldArea = this.mNodes[child2].aabb.perimeter;
        let newArea = aabb.perimeter;
        cost2 = newArea - oldArea + inheritanceCost;
      }
      if (cost < cost1 && cost < cost2) {
        break;
      }
      if (cost1 < cost2) {
        index = child1;
      } else {
        index = child2;
      }
    }
    let sibling = index;
    let oldParent = this.mNodes[sibling].parentOrNext;
    let newParent = this.allocateNode();
    this.mNodes[newParent].parentOrNext = oldParent;
    this.mNodes[newParent].userData = null;
    this.mNodes[newParent].aabb.combine(leafAaBb, this.mNodes[sibling].aabb);
    this.mNodes[newParent].height = this.mNodes[sibling].height + 1;
    if (oldParent !== b2NullNode) {
      if (this.mNodes[oldParent].child1 === sibling) {
        this.mNodes[oldParent].child1 = newParent;
      } else {
        this.mNodes[oldParent].child2 = newParent;
      }
      this.mNodes[newParent].child1 = sibling;
      this.mNodes[newParent].child2 = leaf;
      this.mNodes[sibling].parentOrNext = newParent;
      this.mNodes[leaf].parentOrNext = newParent;
    } else {
      this.mNodes[newParent].child1 = sibling;
      this.mNodes[newParent].child2 = leaf;
      this.mNodes[sibling].parentOrNext = newParent;
      this.mNodes[leaf].parentOrNext = newParent;
      this.mRoot = newParent;
    }
    index = this.mNodes[leaf].parentOrNext;
    while (index !== b2NullNode) {
      index = this.balance(index);
      let child1 = this.mNodes[index].child1;
      let child2 = this.mNodes[index].child2;
      this.mNodes[index].height = 1 + Math.max(this.mNodes[child1].height, this.mNodes[child2].height);
      this.mNodes[index].aabb.combine(this.mNodes[child1].aabb, this.mNodes[child2].aabb);
      index = this.mNodes[index].parentOrNext;
    }
  }

  removeLeaf(leaf: number): void {
    if (leaf === this.mRoot) {
      this.mRoot = b2NullNode;
      return;
    }
    let parent = this.mNodes[leaf].parentOrNext;
    let grandParent = this.mNodes[parent].parentOrNext;
    let sibling: number;
    if (this.mNodes[parent].child1 === leaf) {
      sibling = this.mNodes[parent].child2;
    } else {
      sibling = this.mNodes[parent].child1;
    }
    if (grandParent !== b2NullNode) {
      if (this.mNodes[grandParent].child1 === parent) {
        this.mNodes[grandParent].child1 = sibling;
      } else {
        this.mNodes[grandParent].child2 = sibling;
      }
      this.mNodes[sibling].parentOrNext = grandParent;
      this.freeNode(parent);
      let index = grandParent;
      while (index !== b2NullNode) {
        index = this.balance(index);
        let child1 = this.mNodes[index].child1;
        let child2 = this.mNodes[index].child2;
        this.mNodes[index].aabb.combine(this.mNodes[child1].aabb, this.mNodes[child2].aabb);
        this.mNodes[index].height = 1 + Math.max(this.mNodes[child1].height, this.mNodes[child2].height);
        index = this.mNodes[index].parentOrNext;
      }
    } else {
      this.mRoot = sibling;
      this.mNodes[sibling].parentOrNext = b2NullNode;
      this.freeNode(parent);
    }
  }

  balance(iA: number): number {
    let mA = this.mNodes[iA];
    if (mA.isLeaf() || mA.height < indexTwo) {
      return iA;
    }
    let iB = mA.child1;
    let iC = mA.child2;
    let mB = this.mNodes[iB];
    let mC = this.mNodes[iC];
    let balance = mC.height - mB.height;
    if (balance > 1) {
      let iF = mC.child1;
      let iG = mC.child2;
      let mF = this.mNodes[iF];
      let mG = this.mNodes[iG];
      mC.child1 = iA;
      mC.parentOrNext = mA.parentOrNext;
      mA.parentOrNext = iC;
      if (mC.parentOrNext !== b2NullNode) {
        if (this.mNodes[mC.parentOrNext].child1 === iA) {
          this.mNodes[mC.parentOrNext].child1 = iC;
        } else {
          this.mNodes[mC.parentOrNext].child2 = iC;
        }
      } else {
        this.mRoot = iC;
      }
      if (mF.height > mG.height) {
        mC.child2 = iF;
        mA.child2 = iG;
        mG.parentOrNext = iA;
        mA.aabb.combine(mB.aabb, mG.aabb);
        mC.aabb.combine(mA.aabb, mF.aabb);
        mA.height = 1 + Math.max(mB.height, mG.height);
        mC.height = 1 + Math.max(mA.height, mF.height);
      } else {
        mC.child2 = iG;
        mA.child2 = iF;
        mF.parentOrNext = iA;
        mA.aabb.combine(mB.aabb, mF.aabb);
        mC.aabb.combine(mA.aabb, mG.aabb);
        mA.height = 1 + Math.max(mB.height, mF.height);
        mC.height = 1 + Math.max(mA.height, mG.height);
      }
      return iC;
    }
    if (balance < -1) {
      let iD = mB.child1;
      let iE = mB.child2;
      let mD = this.mNodes[iD];
      let mE = this.mNodes[iE];
      mB.child1 = iA;
      mB.parentOrNext = mA.parentOrNext;
      mA.parentOrNext = iB;
      if (mB.parentOrNext !== b2NullNode) {
        if (this.mNodes[mB.parentOrNext].child1 === iA) {
          this.mNodes[mB.parentOrNext].child1 = iB;
        } else {
          this.mNodes[mB.parentOrNext].child2 = iB;
        }
      } else {
        this.mRoot = iB;
      }
      if (mD.height > mE.height) {
        mB.child2 = iD;
        mA.child1 = iE;
        mE.parentOrNext = iA;
        mA.aabb.combine(mC.aabb, mE.aabb);
        mB.aabb.combine(mA.aabb, mD.aabb);
        mA.height = 1 + Math.max(mC.height, mE.height);
        mB.height = 1 + Math.max(mA.height, mD.height);
      } else {
        mB.child2 = iE;
        mA.child1 = iD;
        mD.parentOrNext = iA;
        mA.aabb.combine(mC.aabb, mD.aabb);
        mB.aabb.combine(mA.aabb, mE.aabb);
        mA.height = 1 + Math.max(mC.height, mD.height);
        mB.height = 1 + Math.max(mA.height, mE.height);
      }
      return iB;
    }
    return iA;
  }

  validateStructure(index: number): void {
    if (index === b2NullNode) {
      return;
    }
    let node = this.mNodes[index];
    let child1 = node.child1;
    let child2 = node.child2;
    if (node.isLeaf()) {
      return;
    }
    this.validateStructure(child1);
    this.validateStructure(child2);
  }
  validateMetrics(index: number): void {
    if (index === b2NullNode) {
      return;
    }
    let node = this.mNodes[index];
    let child1 = node.child1;
    let child2 = node.child2;
    if (node.isLeaf()) {
      return;
    }
    let aabb = new B2AaBb();
    aabb.combine(this.mNodes[child1].aabb, this.mNodes[child2].aabb);
    this.validateMetrics(child1);
    this.validateMetrics(child2);
  }
}
