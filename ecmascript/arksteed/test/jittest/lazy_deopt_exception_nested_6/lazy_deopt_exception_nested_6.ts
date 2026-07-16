/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

declare function print(value: number): void;

declare class ArkTools {
  static arkSteedCompileSync<T extends Function>(func: T): T;
}

function leaf(mode: number, seed: number): number {
  if (mode == 2) {
    throw seed + 11;
  }
  return seed + 4;
}

function bothLazyWithBranches(mode: number, bias: number): number {
  let acc = bias;
  let branch = 0;
  try {
    acc += 1;
    if (mode == 0) {
      branch = acc + 2;
    } else {
      branch = acc + 5;
    }
    try {
      acc += leaf(mode, branch);
      if (branch > 10) {
        acc += 7;
      } else {
        acc += 9;
      }
    } catch (e) {
      acc += branch * 10;
      throw (e as number) + acc;
    }
    acc += branch;
  } catch (e) {
    return 2000 + acc + branch + (e as number);
  }
  return acc;
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  bothLazyWithBranches(0, 8);
}

print(bothLazyWithBranches(0, 8));
ArkTools.arkSteedCompileSync(leaf);
ArkTools.arkSteedCompileSync(bothLazyWithBranches);
print(bothLazyWithBranches(2, 8));
