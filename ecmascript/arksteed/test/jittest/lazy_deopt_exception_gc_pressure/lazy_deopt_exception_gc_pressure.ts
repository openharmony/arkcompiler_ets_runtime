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
  static forceFullGC(): void;
}

let shouldThrow = false;

function callee(): number {
  if (shouldThrow) {
    throw 5;
  }
  return 17;
}

function caller(): number {
  try {
    return callee() + 1;
  } catch (e) {
    return 200 + (e as number);
  }
}

function makeMethod(seed: number): () => number {
  return function generated(): number {
    return seed + 1;
  };
}

function createMethodPressure(rounds: number): number {
  let sum = 0;
  for (let i = 0; i < rounds; i++) {
    const fn = makeMethod(i);
    sum += fn();
    if ((i & 255) === 0) {
      ArkTools.forceFullGC();
    }
  }
  return sum;
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller();
}

print(caller());
print(createMethodPressure(4096) > 0 ? 1 : 0);

ArkTools.forceFullGC();
ArkTools.arkSteedCompileSync(caller);

shouldThrow = true;
print(caller());
