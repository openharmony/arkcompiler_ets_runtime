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
const outsideA = 10;
const outsideB = 7;

function f1(flag: boolean, base: number): number {
  if (flag) {
    throw { branch: 1, code: 31, saved: base };
  }
  return base + 3;
}

function f2(flag: boolean, base: number): number {
  if (flag) {
    throw { branch: 2, code: 41, saved: base };
  }
  return base + 5;
}

function caller(cond: boolean): number {
  const liveBeforeTry = outsideA + outsideB;
  let x = liveBeforeTry;
  try {
    if (cond) {
      x = f1(shouldThrow, liveBeforeTry);
    } else {
      x = f2(shouldThrow, liveBeforeTry);
    }
    return x + liveBeforeTry;
  } catch (e) {
    const obj = e as any;
    return liveBeforeTry * 100 + x * 10 + obj.branch + obj.code + obj.saved;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller((i & 1) === 0);
}

print(caller(true));
print(caller(false));
ArkTools.arkSteedCompileSync(f1);
ArkTools.arkSteedCompileSync(f2);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller(true));
print(caller(false));
