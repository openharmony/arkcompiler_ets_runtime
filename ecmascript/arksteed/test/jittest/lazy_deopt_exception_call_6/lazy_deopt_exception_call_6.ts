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
const outsideA = 3;
const outsideB = 4;
const rightKey = "right";

function leaf(flag: boolean, seed: number): number {
  if (flag) {
    throw { code: 31, payload: { left: 11, right: 13 }, scale: 2 };
  }
  return seed + 2;
}

function bridgeA(flag: boolean, seed: number): number {
  return leaf(flag, seed) + 3;
}

function bridgeB(flag: boolean, seed: number): number {
  return bridgeA(flag, seed) * 2;
}

function bridgeC(flag: boolean, seed: number): number {
  return bridgeB(flag, seed) - 4;
}

function bridgeD(flag: boolean, seed: number): number {
  return bridgeC(flag, seed) + 5;
}

function caller(): number {
  const liveA = outsideA + outsideB;
  const liveB = liveA * outsideB;
  try {
    return bridgeD(shouldThrow, liveB) + liveA;
  } catch (e) {
    const obj = e as any;
    return liveB * 100 + obj.code + obj.payload.left + obj.payload[rightKey] * obj.scale + liveA;
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
ArkTools.arkSteedCompileSync(bridgeD);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller());
