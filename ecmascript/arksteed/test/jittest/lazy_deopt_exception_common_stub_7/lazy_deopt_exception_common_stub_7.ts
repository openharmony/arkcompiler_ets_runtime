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

function defineFieldBranchCase(obj: any, cond: boolean): number {
  let live = 60;
  let marker = 4;
  try {
    for (let i = 0; i < 3; i++) {
      live = live + i;
      if ((i === 1) === cond) {
        obj["k" + i] = live;
        marker = marker + obj["k" + i];
      } else {
        marker = marker + i;
      }
    }
    return live + marker;
  } catch (e) {
    return 1600 + live + marker;
  }
}

const hotObj = {};
const REP = 60000;
for (let i = 0; i < REP; i++) {
  defineFieldBranchCase(hotObj, true);
}

print(defineFieldBranchCase(hotObj, true));
ArkTools.arkSteedCompileSync(defineFieldBranchCase);
print(defineFieldBranchCase(null, true));
