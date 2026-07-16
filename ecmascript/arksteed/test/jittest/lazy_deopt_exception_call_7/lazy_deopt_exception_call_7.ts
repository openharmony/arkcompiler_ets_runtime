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

class Payload {
  code: number;
  extra: number;
  nested: { value: number };
  values: number[];

  constructor(code: number, extra: number, nestedValue: number) {
    this.code = code;
    this.extra = extra;
    this.nested = { value: nestedValue };
    this.values = [code, extra, nestedValue];
  }
}

let shouldThrow = false;
const shared = { factor: 3 };

function leaf(flag: boolean, seed: number): number {
  if (flag) {
    throw new Payload(41, 6, 9);
  }
  return seed + shared.factor;
}

function bridgeA(flag: boolean, seed: number): number {
  return leaf(flag, seed) + 1;
}

function bridgeB(flag: boolean, seed: number): number {
  return bridgeA(flag, seed) * 2;
}

function bridgeC(flag: boolean, seed: number): number {
  return bridgeB(flag, seed) - 3;
}

function bridgeD(flag: boolean, seed: number): number {
  return bridgeC(flag, seed) + 4;
}

function bridgeE(flag: boolean, seed: number): number {
  return bridgeD(flag, seed) * 2;
}

function bridgeF(flag: boolean, seed: number): number {
  return bridgeE(flag, seed) - 5;
}

function caller(): number {
  const liveA = 12;
  const liveB = 5;
  const liveC = liveA * liveB;
  try {
    return bridgeF(shouldThrow, liveC) + liveA + liveB;
  } catch (e) {
    const payload = e as Payload;
    return liveC * 10 + payload.code + payload.extra + payload.nested.value + payload.values[1] + liveA + liveB;
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
ArkTools.arkSteedCompileSync(bridgeE);
ArkTools.arkSteedCompileSync(bridgeF);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller());
