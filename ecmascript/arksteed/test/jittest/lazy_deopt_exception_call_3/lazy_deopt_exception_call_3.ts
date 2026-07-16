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
const outsideBase = 31;
let outsideDelta = 4;

function callee(flag: boolean, payload: number): number {
  if (flag) {
    throw payload + 2;
  }
  return payload + 1;
}

function caller(): number {
  const liveBeforeTry = outsideBase + outsideDelta;
  try {
    const callResult = callee(shouldThrow, liveBeforeTry);
    return callResult + outsideBase + outsideDelta;
  } catch (e) {
    return outsideBase * 10 + outsideDelta + (e as number);
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller();
}

print(caller());
ArkTools.arkSteedCompileSync(callee);
ArkTools.arkSteedCompileSync(caller);
shouldThrow = true;
print(caller());
