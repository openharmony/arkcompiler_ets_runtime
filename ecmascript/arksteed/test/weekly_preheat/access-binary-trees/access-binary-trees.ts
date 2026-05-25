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

const COMMON_NUMBER_TWO: number = 2;
const CURRENT_LOOP_COUNT_NUMBER_TWO: number = 7;
const MAX_LOOP_COUNT: number = 80;
const MS_CONVERSION_RATIO: number = 1000;

class TreeNode {
  left: TreeNode | null;
  right: TreeNode | null;
  item: number;

  constructor(left: TreeNode, right: TreeNode, item: number) {
    this.left = left;
    this.right = right;
    this.item = item;
  }

  itemCheck(): number {
    if (this.left === null) {
      return this.item;
    } else {
      return this.item + (this.left.itemCheck() ?? 0.0) - (this.right?.itemCheck() ?? 0.0);
    }
  }
}

function bottomUpTree(item: number, depth: number): TreeNode {
  if (depth > 0) {
    return new TreeNode(bottomUpTree(COMMON_NUMBER_TWO * item - 1, depth - 1), bottomUpTree(COMMON_NUMBER_TWO * item, depth - 1), item);
  } else {
    return new TreeNode(null, null, item);
  }
}

declare interface ArkTools {
  timeInUs(args: number): number;
}

/*
 *@State
 */
class Benchmark {
  run(): void {
    let ret = 0.0;
    for (let n = 4; n <= CURRENT_LOOP_COUNT_NUMBER_TWO; n++) {
      let minDepth = 4;
      let maxDepth = Math.max(minDepth + COMMON_NUMBER_TWO, n);
      let stretchDepth = maxDepth + 1;

      let check = bottomUpTree(0, stretchDepth).itemCheck();

      let longLivedTree = bottomUpTree(0, maxDepth);
      for (let depth = minDepth; depth < maxDepth; depth += COMMON_NUMBER_TWO) {
        let iterations = 1 << (maxDepth - depth + minDepth);

        check = 0;
        for (let i = 1; i <= iterations; i++) {
          check += bottomUpTree(i, depth).itemCheck();
          check += bottomUpTree(-i, depth).itemCheck();
        }
      }
      ret += longLivedTree.itemCheck();
    }

    let expected: number = -4;
    if (ret !== expected) {
      print('ERROR: bad result: expected' + expected + ' but got' + ret);
    }
  }

  /*
   * @Benchmark
   */
  runIterationTime(): void {
    let start = ArkTools.timeInUs();
    for (let i = 0; i < MAX_LOOP_COUNT; i++) {
      this.run();
    }
    let end = ArkTools.timeInUs();
    print('access-binary-trees: ms = ' + (end - start) / MS_CONVERSION_RATIO);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().runIterationTime();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().runIterationTime();
