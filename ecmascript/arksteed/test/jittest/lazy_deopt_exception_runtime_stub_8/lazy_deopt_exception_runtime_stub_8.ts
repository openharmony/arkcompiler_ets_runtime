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

function arraySpreadCase(src: any): number {
  let live = 110;
  let marker = 9;
  try {
    for (let i = 0; i < 2; i++) {
      live = live + i;
    }
    const arr = [marker, ...src];
    marker = arr[0] + arr[1] + arr.length;
    return live + marker;
  } catch (e) {
    return 2100 + live + marker;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  arraySpreadCase([19, 1]);
}

print(arraySpreadCase([19, 1]));
ArkTools.arkSteedCompileSync(arraySpreadCase);
print(arraySpreadCase(4));
