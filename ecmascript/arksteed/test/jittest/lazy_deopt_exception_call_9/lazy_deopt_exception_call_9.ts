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
const outside = 6;

function hotA(flag: boolean, seed: number): number {
  if (flag) {
    throw { branch: 11, code: 50, seed };
  }
  return seed + 1;
}

function hotB(flag: boolean, seed: number): number {
  if (flag) {
    throw { branch: 12, code: 60, seed };
  }
  return seed + 2;
}

function coldC(flag: boolean, seed: number): number {
  if (flag) {
    throw { branch: 13, code: 70, seed };
  }
  return seed + 3;
}

function caller(mode: number): number {
  const liveBeforeTry = outside * 4;
  let x = liveBeforeTry;
  let y = outside;
  try {
    if (mode === 0) {
      x = hotA(shouldThrow, liveBeforeTry);
      y = x + 10;
    } else {
      if (mode === 1) {
        x = hotB(shouldThrow, liveBeforeTry + 1);
        y = x + 20;
      } else {
        x = coldC(shouldThrow, liveBeforeTry + 2);
        y = x + 30;
      }
    }
    return x + y + liveBeforeTry;
  } catch (e) {
    const obj = e as any;
    return liveBeforeTry * 100 + x * 10 + y + obj.branch + obj.code + obj.seed;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller(i % 3);
}

print(caller(0));
print(caller(1));
print(caller(2));
ArkTools.arkSteedCompileSync(hotA);
ArkTools.arkSteedCompileSync(hotB);
ArkTools.arkSteedCompileSync(coldC);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller(0));
print(caller(1));
print(caller(2));
