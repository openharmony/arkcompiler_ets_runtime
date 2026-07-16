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

function leaf(flag: boolean): number {
  if (flag) {
    throw 13;
  }
  return 5;
}

function middleA(flag: boolean): number {
  return leaf(flag) + 1;
}

function middleB(flag: boolean): number {
  return middleA(flag) * 2;
}

function caller(flag: boolean): number {
  try {
    return middleB(flag) + 3;
  } catch (e) {
    return 700 + (e as number);
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller(false);
}

print(caller(false));
ArkTools.arkSteedCompileSync(leaf);
ArkTools.arkSteedCompileSync(middleA);
ArkTools.arkSteedCompileSync(middleB);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller(shouldThrow));
