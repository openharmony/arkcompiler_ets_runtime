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

function deleteCase(obj: any, rounds: number): number {
  let live = 50;
  let count = 2;
  try {
    for (let i = 0; i < rounds; i++) {
      live = live + i;
      if (i === 1) {
        delete obj.a;
      } else {
        count = count + obj.b;
      }
    }
    return live + count;
  } catch (e) {
    return 1500 + live + count;
  }
}

const hotObj = { a: 1, b: 3 };
const REP = 60000;
for (let i = 0; i < REP; i++) {
  deleteCase(hotObj, 3);
  hotObj.a = 1;
}

print(deleteCase(hotObj, 3));
ArkTools.arkSteedCompileSync(deleteCase);
print(deleteCase(null, 2));
