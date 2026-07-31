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

declare function print(value: number | string): void;

declare class ArkTools {
  static arkSteedCompileSync<T extends Function>(func: T): T;
}

function stringAddNestedBranchCase(left: any, mode: number): string {
  let result = "base";
  let live = "B";
  let guard = 10;
  try {
    if (mode > 1) {
      guard = guard + 2;
      if (mode === 2) {
        live = live + "T" + guard;
        result = left + live;
      } else {
        live = live + "U";
        result = live + left;
      }
    } else if (mode === 1) {
      guard = guard + 1;
      live = live + "M";
      result = live + "middle";
    } else {
      live = live + "F";
      result = "" + live;
    }
    return result + ":tail:" + guard;
  } catch (e) {
    return "catch:" + live + ":" + result + ":" + guard;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  stringAddNestedBranchCase("hot", 0);
}

print(stringAddNestedBranchCase("hot", 0));
ArkTools.arkSteedCompileSync(stringAddNestedBranchCase);
print(stringAddNestedBranchCase(Symbol("lazy"), 2));
