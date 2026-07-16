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

function toNumberLiveCase(value: any): number {
  let live = 31;
  let numeric = 4;
  let extra = 2;
  try {
    live = live + 3;
    numeric = +value;
    extra = numeric - 26;
    return live + extra;
  } catch (e) {
    return 1100 + live + numeric + extra;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  toNumberLiveCase(34);
}

print(toNumberLiveCase(34));
ArkTools.arkSteedCompileSync(toNumberLiveCase);
print(toNumberLiveCase(Symbol("lazy")));
