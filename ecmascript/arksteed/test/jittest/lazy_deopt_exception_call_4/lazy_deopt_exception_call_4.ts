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
const outsideA = 6;
const outsideB = 8;

function leaf(flag: boolean, base: number): number {
  if (flag) {
    throw 19;
  }
  return base + 1;
}

function bridgeA(flag: boolean, base: number): number {
  return leaf(flag, base) + 2;
}

function bridgeB(flag: boolean, base: number): number {
  return bridgeA(flag, base) * 3;
}

function bridgeC(flag: boolean, base: number): number {
  return bridgeB(flag, base) - 5;
}

function caller(): number {
  const liveBeforeTry = outsideA + outsideB;
  try {
    return bridgeC(shouldThrow, liveBeforeTry) + liveBeforeTry;
  } catch (e) {
    return liveBeforeTry * 100 + (e as number);
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller();
}

print(caller());
ArkTools.arkSteedCompileSync(leaf);
ArkTools.arkSteedCompileSync(bridgeA);
ArkTools.arkSteedCompileSync(bridgeB);
ArkTools.arkSteedCompileSync(bridgeC);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller());
