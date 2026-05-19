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

const SPLAY_8000: number = 8000;
const SPLAY_80: number = 80;
const SPLAY_5: number = 5;
const SPLAY_1000: number = 1000;
const SPLAY_2: number = 2;
const SPLAY_3: number = 3;
const SPLAY_4: number = 4;
const SPLAY_6: number = 6;
const SPLAY_7: number = 7;
const SPLAY_8: number = 8;
const SPLAY_9: number = 9;
const SPLAY_120: number = 20;
const SPLAY_50: number = 50;
const SPLAY_20: number = 20;
const SPLAY_19: number = 19;

class Performance {
  static now(): number {
    return ArkTools.timeInUs() / SPLAY_1000;
  }
}

class PayloadTree0 {
  array: number[];
  str: string;
  constructor(array: number[], str: string) {
    this.array = array;
    this.str = str;
  }
}

class PayloadTreeDepth {
  left: PayloadTreeDepth;
  right: PayloadTreeDepth;
  constructor(left: PayloadTreeDepth, right: PayloadTreeDepth) {
    this.left = left;
    this.right = right;
  }
}

class Splay {
  // Configuration.
  splayTree: SplayTree | null = new SplayTree();
  splaySampleTimeStart: number = 0.0;
  splaySamples: number[] = [];

  generatePayloadTree(depth: number, tag: number | string): PayloadTree0 | PayloadTreeDepth {
    if (depth === 0) {
      return new PayloadTree0([0, 1, SPLAY_2, SPLAY_3, SPLAY_4, SPLAY_5, SPLAY_6, SPLAY_7, SPLAY_8, SPLAY_9], 'String for key ' + tag + ' in leaf node');
    } else {
      return new PayloadTreeDepth(this.generatePayloadTree(depth - 1, tag) as PayloadTreeDepth, this.generatePayloadTree(depth - 1, tag) as PayloadTreeDepth);
    }
  }

  generateKey(): number {
    // The benchmark framework guarantees that Math.random is
    // deterministic; see base.js.
    return Math.random();
  }

  splayUpdateStats(time: number): void {
    const pause: number = time - this.splaySampleTimeStart;
    this.splaySampleTimeStart = time;
    this.splaySamples.push(pause);
  }

  insertNewNode(): number {
    // Insert new node with a unique key.
    let key: number;
    do {
      key = this.generateKey();
    } while ((this.splayTree && this.splayTree.find(key)) !== null);
    const payload = this.generatePayloadTree(SPLAY_5, String(key));
    this.splayTree!.insert(key, payload);
    return key;
  }
  /*
   * @Setup
   */
  splaySetup(): void {
    // Check if the platform has the performance.now high resolution timer.
    // If not, throw exception and quit.
    if (!Performance.now) {
      //deBugLog("PerformanceNowUnsupported")
      return;
    }
    this.splayTree = new SplayTree();
    this.splaySampleTimeStart = Performance.now();
    for (let i = 0; i < SPLAY_8000; i++) {
      this.insertNewNode();
      if ((i + 1) % SPLAY_20 === SPLAY_19) {
        this.splayUpdateStats(Performance.now());
      }
    }
  }
  /*
   * @Teardown
   */
  splayTearDown(): void {
    // Allow the garbage collector to reclaim the memory
    // used by the splay tree no matter how we exit the
    // tear down function.
    const keys = this.splayTree!.exportKeys();
    this.splayTree = null;
    this.splaySamples = [];
    // Verify that the splay tree has the right size.
    const length = keys.length;
    if (length !== SPLAY_8000) {
      return;
    }
    // Verify that the splay tree has sorted, unique keys.
    for (let i = 0; i < length - 1; i++) {
      if (keys[i] >= keys[i + 1]) {
        return;
      }
    }
  }
  /*
   * @Benchmark
   */
  splayRun(): void {
    // Replace a few nodes in the splay tree.
    for (let i = 0; i < SPLAY_80; i++) {
      const key = this.insertNewNode();
      const greatest = this.splayTree!.findGreatestLessThan(key);
      if (greatest === null) {
        this.splayTree!.remove(key);
      } else {
        this.splayTree!.remove(greatest.key as number);
      }
    }
    this.splayUpdateStats(Performance.now());
  }
}

/**
 ** Constructs a Splay tree.  A splay tree is a self-balancing binary
 ** search tree with the additional property that recently accessed
 ** elements are quick to access again. It performs basic operations
 ** such as insertion, look-up and removal in O(log(n)) amortized time.
 **
 ** @constructor
 **/
class SplayTree {
  /**
   * Pointer to the root node of the tree.
   *
   * @type {SplayTree.Node}
   * @private
   */
  root: Node | null = null;
  /**
   * @return {boolean} Whether the tree is empty.
   */
  isEmpty(): boolean {
    return !this.root;
  }
  /**
   ** Inserts a node into the tree with the specified key and value if
   ** the tree does not already contain a node with the specified key. If
   ** the value is inserted, it becomes the root of the tree.
   *
   ** @param {number} key Key to insert into the tree.
   ** @param {*} value Value to insert into the tree.
   **/
  insert(key: number, value: PayloadTree0 | PayloadTreeDepth | null): void {
    if (this.isEmpty()) {
      this.root = new Node(key, value);
      return;
    }
    // Splay on the key to move the last node on the search path for
    // the key to the root of the tree.
    this.splay(key);
    if (this.root?.key === key) {
      return;
    }
    let node = new Node(key, value);
    if (key > Number(this.root!.key)) {
      node.left = this.root!;
      node.right = this.root!.right;
      this.root!.right = null;
    } else {
      node.right = this.root!;
      node.left = this.root!.left;
      this.root!.left = null;
    }
    this.root = node;
  }
  /**
   ** Removes a node with the specified key from the tree if the tree
   ** contains a node with this key. The removed node is returned. If the
   ** key is not found, an exception is thrown.
   *
   ** @param {number} key Key to find and remove from the tree.
   ** @return {SplayTree.Node} The removed node.
   **/
  remove(key: number): Node | null {
    if (this.isEmpty()) {
      return null;
    }
    this.splay(key);
    if (this.root!.key !== key) {
      return null;
    }
    const removed = this.root;
    if (!this.root!.left) {
      this.root = this.root!.right;
    } else {
      const right = this.root!.right;
      this.root = this.root!.left;
      // Splay to make sure that the new root has an empty right child.
      this.splay(key);
      // Insert the original right child as the right child of the new root.
      this.root.right = right;
    }
    return removed!;
  }
  /**
   * Returns the node having the specified key or null if the tree doesn't contain
   * a node with the specified key.
   *
   * @param {number} key Key to find in the tree.
   * @return {SplayTree.Node} Node having the specified key.
   */
  find(key: number): Node | null {
    if (this.isEmpty()) {
      return null;
    }
    this.splay(key);
    return this.root?.key === key ? this.root : null;
  }
  /**
   * @return {SplayTree.Node} Node having the maximum key value.
   */
  findMax(optStartNode: Node | null = null): Node | null {
    if (this.isEmpty()) {
      return null;
    }
    let current = optStartNode || this.root;
    while (current!.right) {
      current = current!.right;
    }
    return current;
  }
  /**
   * @return {SplayTree.Node} Node having the maximum key value that
   *     is less than the specified key value.
   */
  findGreatestLessThan(key: number): Node | null {
    if (this.isEmpty()) {
      return null;
    }
    // Splay on the key to move the node with the given key or the last
    // node on the search path to the top of the tree.
    this.splay(key);
    // Now the result is either the root node or the greatest node in
    // the left subtree.
    if (Number(this.root!.key) < key) {
      return this.root;
    } else if (this.root!.left) {
      return this.findMax(this.root!.left);
    } else {
      return null;
    }
  }
  /**
   * @return {Array<*>} An array containing all the keys of tree's nodes.
   */
  exportKeys(): number[] {
    const result: number[] = [];
    if (!this.isEmpty()) {
      this.root!.traverse(node => {
        result.push(node.key as number);
      });
    }
    return result;
  }
  /**
   ** Perform the splay operation for the given key. Moves the node with
   ** the given key to the top of the tree.  If no node has the given
   ** key, the last node on the search path is moved to the top of the
   ** tree. This is the simplified top-down splaying algorithm from:
   ** "Self-adjusting Binary Search Trees" by Sleator and Tarjan
   *
   ** @param {number} key Key to splay the tree on.
   ** @private
   **/
  splay(key: number): void {
    if (this.isEmpty()) {
      return;
    }
    let dummy: Node;
    let left: Node;
    let right: Node;
    dummy = new Node(0, null);
    left = dummy;
    right = dummy;
    let current = this.root;
    while (true) {
      if (key < Number(current!.key)) {
        if (!current!.left) {
          break;
        }
        if (key < Number(current!.left.key)) {
          let temp = current!.left;
          current!.left = temp.right;
          temp.right = current;
          current = temp;
          if (!current.left) {
            break;
          }
        }
        right.left = current;
        right = current!;
        current = current!.left;
      } else if (key > Number(current!.key)) {
        if (!current!.right) {
          break;
        }
        if (key > Number(current!.right.key)) {
          let temp = current!.right;
          current!.right = temp.left;
          temp.left = current;
          current = temp;
          if (!current.right) {
            break;
          }
        }
        left.right = current;
        left = current!;
        current = current!.right;
      } else {
        break;
      }
    }
    // Assemble.
    left.right = current!.left;
    right.left = current!.right;
    current!.left = dummy.right;
    current!.right = dummy.left;
    this.root = current;
  }
}

/**
 * Constructs a Splay tree node.
 *
 * @param {number} key Key.
 * @param {*} value Value.
 */
class Node {
  key: number | null;
  value: object | null;
  left: Node | null;
  right: Node | null;

  constructor(key: number | null, value: PayloadTree0 | PayloadTreeDepth | null) {
    this.key = key;
    this.value = value;
    this.right = null;
    this.left = null;
  }
  /**
   * Performs an ordered traversal of the subtree starting at
   * this SplayTree.Node.
   *
   * @param {function(SplayTree.Node)} f Visitor function.
   * @private
   */
  traverse(f: (node: Node) => void): void {
    traverseSub(this, f);
  }
}
function traverseSub(current: Node | null, f: (node: Node) => void): void {
  while (current) {
    let left = current.left;
    if (left) {
      left.traverse(f);
    }
    f(current);
    current = current.right;
  }
}

function deBugLog(str: string): void {
  let isLog = false;
  if (isLog) {
    print(str);
  }
}

function splayRunIteration(): void {
  let setupStart = Performance.now();
  let splay = new Splay();
  splay.splaySetup();
  let setupEnd = Performance.now();
  //deBugLog("setupTime: " + String(setupEnd - setupStart) + "ms")
  for (let i = 0; i < SPLAY_120; ++i) {
    for (let i = 0; i < SPLAY_50; ++i) {
      splay.splayRun();
    }
  }
  splay.splayTearDown();
  let splayEnd = Performance.now();
  print('splay: ms = ' + String(splayEnd - setupStart));
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  splayRunIteration();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

splayRunIteration();
