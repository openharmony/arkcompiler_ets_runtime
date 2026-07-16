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

function leaf(mode: number, seed: number): number {
  if (mode == 1) {
    throw seed + 5;
  }
  return seed + 2;
}

function innerLazyOnly(mode: number, bias: number): number {
  let acc = bias;
  try {
    acc += 2;
    try {
      acc += leaf(mode, acc);
      acc += 4;
    } catch (e) {
      acc += 200;
      return acc + (e as number);
    }
    acc += 6;
  } catch (e) {
    return 900 + acc + (e as number);
  }
  return acc;
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  innerLazyOnly(0, 5);
}

print(innerLazyOnly(0, 5));
ArkTools.arkSteedCompileSync(leaf);
ArkTools.arkSteedCompileSync(innerLazyOnly);
print(innerLazyOnly(1, 5));
