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

function booleanBranchCase(value: any, rounds: number): number {
  let live = 70;
  let score = 5;
  try {
    for (let i = 0; i < rounds; i++) {
      live = live + 1;
      if (value) {
        score = score + i + 2;
      } else {
        score = score - i;
      }
    }
    return live + score;
  } catch (e) {
    return 1700 + live + score;
  }
}

const throwingBool = {
  valueOf(): boolean {
    throw 1;
  }
};

const REP = 60000;
for (let i = 0; i < REP; i++) {
  booleanBranchCase("x", 3);
}

print(booleanBranchCase("x", 3));
ArkTools.arkSteedCompileSync(booleanBranchCase);
print(booleanBranchCase(throwingBool, 2));
