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

let shouldThrow = false;
const outside = 17;
const bonusKey = "bonus";

function leaf(flag: boolean, payload: number): number {
  if (flag) {
    throw { code: 23, bonus: 5, nested: { value: 7 } };
  }
  return payload + 1;
}

function middleA(flag: boolean, payload: number): number {
  return leaf(flag, payload) + 2;
}

function middleB(flag: boolean, payload: number): number {
  return middleA(flag, payload) * 2;
}

function caller(): number {
  const liveBeforeTry = outside + 3;
  try {
    return middleB(shouldThrow, liveBeforeTry) + liveBeforeTry;
  } catch (e) {
    const obj = e as any;
    return liveBeforeTry * 100 + obj.code + obj[bonusKey] + obj.nested.value;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller();
}

print(caller());
ArkTools.arkSteedCompileSync(leaf);
ArkTools.arkSteedCompileSync(middleA);
ArkTools.arkSteedCompileSync(middleB);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller());
