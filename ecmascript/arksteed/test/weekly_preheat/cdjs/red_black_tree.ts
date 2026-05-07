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
import { Vector2D } from './vector_2d';
import { Vector3D } from './vector_3d';
import { CallSign } from './call_sign';
import { Motion } from './motion';

class ArrayResult {
  key: Vector2D | CallSign;
  value: Vector3D | boolean | Array<Motion>;

  constructor(key: Vector2D | CallSign, value: Vector3D | boolean | Array<Motion>) {
    this.key = key;
    this.value = value;
  }
}

class InsertionResult {
  isNewEntry: boolean;
  oldValue: Vector3D | boolean | Array<Motion> | null;
  newNode: Node | null;

  constructor(isNewEntry: boolean, oldValue: Vector3D | boolean | Array<Motion> | null, newNode: Node | null) {
    this.isNewEntry = isNewEntry;
    this.oldValue = oldValue;
    this.newNode = newNode;
  }
}

function compare(a: Vector2D | CallSign, b: Vector2D | CallSign): number {
  if (a instanceof Vector2D && b instanceof Vector2D) {
    return a.compareTo(b);
  }
  if (a instanceof CallSign && b instanceof CallSign) {
    return a.compareTo(b);
  }
  print('Mismatched types');
  return 0;
}

function treeMinimum(x: Node): Node {
  let y = x;
  while (y.left !== null) {
    y = y.left!;
  }
  return y;
}

function treeMaximum(x: Node): Node {
  let y = x;
  while (y.right !== null) {
    y = y.right!;
  }
  return y;
}

class Node {
  key: Vector2D | CallSign;
  value: Vector3D | boolean | Array<Motion>;
  left: Node | null = null;
  right: Node | null = null;
  parent: Node | null = null;
  color: string = 'red';

  constructor(key: Vector2D | CallSign, value: Vector3D | boolean | Array<Motion>) {
    this.key = key;
    this.value = value;
  }

  successor(x: Node | null): Node | null {
    if (x.right !== null) {
      return treeMinimum(x.right);
    }
    let y = x.parent;
    while (y !== null && x === y.right) {
      x = y;
      y = y.parent;
    }
    return y;
  }

  predecessor(x: Node | null): Node | null {
    if (x.left !== null) {
      return treeMaximum(x.left);
    }
    let y = x.parent;
    while (y !== null && x === y.left) {
      x = y;
      y = y.parent;
    }
    return y;
  }

  toString(): string {
    return this.key + '=>' + this.value + ' (' + this.color + ')';
  }
}

export class RedBlackTree {
  root: Node | null;

  constructor() {
    this.root = null;
  }

  put(key: Vector2D | CallSign, value: Vector3D | boolean | Array<Motion>): Vector3D | boolean | Array<Motion> | null {
    let insertionResult = this.treeInsert(key, value);
    if (!insertionResult.isNewEntry) {
      return insertionResult.oldValue!;
    }
    let x = insertionResult.newNode;
    while (x !== this.root && x!.parent!.color === 'red') {
      if (x!.parent === x!.parent!.parent!.left) {
        let y = x!.parent!.parent!.right;
        if (y !== null && y.color === 'red') {
          x!.parent!.color = 'black';
          y!.color = 'black';
          x!.parent!.parent!.color = 'red';
          x = x!.parent!.parent!;
        } else {
          if (x === x!.parent!.right) {
            x = x!.parent!;
            this.leftRotate(x);
          }
          x!.parent!.color = 'black';
          x!.parent!.parent!.color = 'red';
          this.rightRotate(x!.parent!.parent!);
        }
      } else {
        // Same as "then" clause with "right" and "left" exchanged.
        if (x !== null) {
          let y = x.parent.parent.left;
          if (y !== null && y!.color === 'red') {
            x.parent!.color = 'black';
            y.color = 'black';
            x.parent.parent.color = 'red';
            x = x.parent.parent;
          } else {
            if (x === x.parent.left) {
              x = x.parent;
              this.rightRotate(x);
            }
            x.parent.color = 'black';
            x.parent.parent.color = 'red';
            this.leftRotate(x.parent.parent);
          }
        }
      }
    }
    this!.root!.color = 'black';
    return null;
  }

  remove(key: Vector2D | CallSign): Vector3D | boolean | Array<Motion> | null {
    let z: Node | null = this.findNode(key);
    if (z === null) {
      return null;
    }

    // Y is the node to be unlinked from the tree.
    let y: Node | null = this.getYByZ(z);
    // Y is guaranteed to be non-null at this point.
    let x: Node | null = this.getXByY(y);

    // X is the child of y which might potentially replace y in the tree. X might be null at
    // this point.
    let xParent: Node | null;
    if (x !== null) {
      x!.parent = y!.parent!;
      xParent = x!.parent;
    } else {
      xParent = y!.parent!;
    }
    if (y!.parent === null) {
      this!.root = x;
    } else {
      if (y === y!.parent!.left) {
        y!.parent!.left = x;
      } else {
        y!.parent!.right = x;
      }
    }

    z = this.recalculateZ(x, y, z, xParent);
    return z!.value;
  }

  getYByZ(z: Node | null): Node | null {
    if (z!.left === null || z!.right == null) {
      return z;
    } else {
      return z!.successor(z);
    }
  }

  getXByY(y: Node | null): Node | null {
    if (y!.left !== null) {
      return y!.left;
    } else {
      return y!.right!;
    }
  }

  recalculateZ(x: Node | null, y: Node | null, z: Node | null, xParent: Node | null): Node | null {
    if (y !== z) {
      if (y!.color === 'black') {
        this.removeFixup(x, xParent);
      }

      y!.parent = z!.parent!;
      y!.color = z!.color!;
      y!.left = z!.left!;
      y!.right = z!.right!;

      if (z!.left !== null) {
        z!.left!.parent = y;
      }
      if (z!.right !== null) {
        z!.right!.parent = y;
      }
      if (z!.parent !== null) {
        if (z!.parent!.left === z) {
          z!.parent!.left = y;
        } else {
          z!.parent!.right = y;
        }
      } else {
        this!.root = y;
      }
    } else if (y!.color === 'black') {
      this.removeFixup(x, xParent);
    }

    return z;
  }

  get(key: Vector2D | CallSign): Vector3D | boolean | Array<Motion> | null {
    let node: Node | null = this.findNode(key);
    if (node === null) {
      return null;
    }
    return node.value;
  }

  forEach(callback: (key: Vector2D | CallSign, value: Vector3D | boolean | Array<Motion>) => void): void {
    if (this.root === null) {
      return;
    }
    let current: Node | null = treeMinimum(this!.root);
    while (current !== null) {
      callback(current!.key, current!.value);
      current = current!.successor(current);
    }
  }

  asArray(): Array<ArrayResult> {
    let result: Array<ArrayResult> = [];
    this.forEach((key: Vector2D | CallSign, value: Vector3D | boolean | Array<Motion>) => {
      result.push(new ArrayResult(key, value));
    });
    return result;
  }

  toString(): string {
    let result = '[';
    let first = true;
    this.forEach((key: Vector2D | CallSign, value: Vector3D | boolean | Array<Motion>) => {
      if (first) {
        first = false;
      } else {
        result += ', ';
      }
      result += key + '=>' + value;
    });
    return result + ']';
  }

  findNode(key: Vector2D | CallSign): Node | null {
    let current = this.root;
    while (current !== null) {
      let comparisonResult = compare(key, current.key);
      if (comparisonResult === 0) {
        return current;
      }
      if (comparisonResult > 0) {
        current = current.right;
      } else {
        current = current.left;
      }
    }
    return null;
  }

  treeInsert(key: Vector2D | CallSign, value: Vector3D | boolean | Array<Motion>): InsertionResult {
    let y: Node | null = null;
    let x = this.root;
    while (x !== null) {
      y = x;
      let comparisonResult: number = 0;
      if (key instanceof Vector2D && x.key instanceof Vector2D) {
        comparisonResult = key.compareTo(x.key);
      } else if (key instanceof CallSign && x.key instanceof CallSign) {
        comparisonResult = key.compareTo(x.key);
      }
      if (comparisonResult < 0) {
        x = x.left;
      } else if (comparisonResult > 0) {
        x = x.right;
      } else {
        let oldValue = x.value;
        x.value = value;
        return new InsertionResult(false, oldValue, null);
      }
    }
    let z = new Node(key, value);
    z!.parent = y;
    if (y === null) {
      this.root = z;
    } else {
      let comparisonResult: number = 0;
      if (key instanceof Vector2D && y.key instanceof Vector2D) {
        comparisonResult = key.compareTo(y.key);
      } else if (key instanceof CallSign && y.key instanceof CallSign) {
        comparisonResult = key.compareTo(y.key);
      }
      if (comparisonResult < 0) {
        y.left = z;
      } else {
        y.right = z;
      }
    }
    return new InsertionResult(true, null, z);
  }

  leftRotate(x: Node): Node {
    let y = x.right!;

    // Turn y's left subtree into x's right subtree.
    x.right = y!.left!;
    if (y!.left !== null) {
      y!.left!.parent = x;
    }

    // Link x's parent to y.
    y!.parent = x.parent;
    if (x.parent === null) {
      this.root = y;
    } else {
      if (x === x.parent.left) {
        x.parent.left = y;
      } else {
        x.parent.right = y;
      }
    }

    // Put x on y's left.
    y!.left = x;
    x.parent = y;

    return y;
  }

  rightRotate(yN: Node): Node {
    let x = yN.left!;

    // Turn x's right subtree into y's left subtree.
    yN.left = x.right;
    if (x.right !== null) {
      x.right.parent = yN;
    }

    // Link y's parent to x;
    x.parent = yN.parent;
    if (yN.parent === null) {
      this.root = x;
    } else {
      if (yN !== yN.parent.left) {
        yN.parent.right = x;
      } else {
        yN.parent.left = x;
      }
    }
    x.right = yN;
    yN.parent = x;
    return x;
  }

  removeFixup(x: Node | null, xParent: Node | null): void {
    let y = x;
    let yParent = xParent;
    while (y !== this.root && (y === null || y.color === 'black')) {
      if (y === yParent!.left) {
        let result = this.xEqualParentLeft(y, yParent);
        y = result[0];
        yParent = result[1];
      } else {
        let result = this.xEqualParentRight(y, yParent);
        y = result[0];
        yParent = result[1];
      }
    }
    if (y !== null) {
      y.color = 'black';
    }
  }

  xEqualParentLeft(x: Node | null, xParent: Node | null): [Node | null, Node | null] {
    // Note: the text points out that w cannot be null. The reason is not obvious from
    // simply looking at the code; it comes about from the properties of the red-black
    // tree.
    let w = xParent!.right;
    if (w!.color === 'red') {
      // Case 1
      w!.color = 'black';
      xParent!.color = 'red';
      this.leftRotate(xParent!);
      w = xParent!.right;
    }
    if ((w!.left === null || w!.left!.color === 'black') && (w!.right === null || w!.right!.color === 'black')) {
      // Case 2
      w!.color = 'red';
      x = xParent;
      xParent = x!.parent!;
    } else {
      if (w!.right === null || w!.right!.color === 'black') {
        // Case 3
        w!.left!.color = 'black';
        w!.color = 'red';
        this.rightRotate(w!);
        w = xParent!.right;
      }
      // Case 4
      w!.color = xParent!.color;
      xParent!.color = 'black';
      if (w!.right !== null) {
        w!.right!.color = 'black';
      }
      this.leftRotate(xParent!);
      x = this.root;
      xParent = x!.parent;
    }

    return [x, xParent];
  }

  xEqualParentRight(x: Node | null, xParent: Node | null): [Node | null, Node | null] {
    // Same as "then" clause with "right" and "left" exchanged.
    let w = xParent!.left;
    if (w!.color === 'red') {
      // Case 1
      w!.color = 'black';
      xParent!.color = 'red';
      this.rightRotate(xParent!);
      w = xParent!.left;
    }
    if ((w!.right === null || w!.right!.color === 'black') && (w!.left === null || w!.left!.color === 'black')) {
      // Case 2
      w!.color = 'red';
      x = xParent;
      xParent = x!.parent;
    } else {
      if (w!.left === null || w!.left.color === 'black') {
        // Case 3
        w!.right!.color = 'black';
        w!.color = 'red';
        this.leftRotate(w!);
        w = xParent!.left;
      }
      // Case 4
      w!.color = xParent!.color;
      xParent!.color = 'black';
      if (w!.left !== null) {
        w!.left!.color = 'black';
      }
      this.rightRotate(xParent!);
      x = this.root;
      xParent = x!.parent;
    }

    return [x, xParent];
  }
}
