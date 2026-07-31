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

function stringAddLoopCase(left: any, cond: boolean): string {
  let result = "base";
  let live = "A";
  let counter = 0;
  try {
    for (let i = 0; i < 3; i++) {
      counter = counter + i;
      live = live + i;
    }
    if (cond) {
      live = live + "T" + counter;
      result = left + live;
    } else {
      live = live + "F";
      result = "" + live;
    }
    return result + ":tail:" + counter;
  } catch (e) {
    return "catch:" + live + ":" + result + ":" + counter;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  stringAddLoopCase("hot", false);
}

print(stringAddLoopCase("hot", false));
ArkTools.arkSteedCompileSync(stringAddLoopCase);
print(stringAddLoopCase(Symbol("lazy"), true));
