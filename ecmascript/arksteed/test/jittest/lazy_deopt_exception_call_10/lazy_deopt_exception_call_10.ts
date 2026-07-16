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
const outside = { base: 9, adjust: 4 };

function f1(flag: boolean, seed: number, sum: number): number {
  if (flag) {
    throw { branch: 21, code: 80, seed, sum };
  }
  return seed + sum + 1;
}

function f2(flag: boolean, seed: number, sum: number): number {
  if (flag) {
    throw { branch: 22, code: 90, seed, sum };
  }
  return seed + sum + 2;
}

function f3(flag: boolean, seed: number, sum: number): number {
  if (flag) {
    throw { branch: 23, code: 100, seed, sum };
  }
  return seed + sum + 3;
}

function caller(mode: number): number {
  const liveBeforeTry = outside.base + outside.adjust;
  let x = liveBeforeTry;
  let sum = 0;
  try {
    for (let i = 0; i < 3; i++) {
      sum += liveBeforeTry + i;
    }
    if (mode < 0) {
      x = f1(shouldThrow, liveBeforeTry, sum);
    } else if (mode === 0) {
      x = f2(shouldThrow, liveBeforeTry + 1, sum);
    } else {
      x = f3(shouldThrow, liveBeforeTry + 2, sum);
    }
    return x + sum + liveBeforeTry;
  } catch (e) {
    const obj = e as any;
    return liveBeforeTry * 1000 + sum * 10 + x + obj.branch + obj.code + obj.seed + obj.sum;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller((i % 3) - 1);
}

print(caller(-1));
print(caller(0));
print(caller(1));
ArkTools.arkSteedCompileSync(f1);
ArkTools.arkSteedCompileSync(f2);
ArkTools.arkSteedCompileSync(f3);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller(-1));
print(caller(0));
print(caller(1));
