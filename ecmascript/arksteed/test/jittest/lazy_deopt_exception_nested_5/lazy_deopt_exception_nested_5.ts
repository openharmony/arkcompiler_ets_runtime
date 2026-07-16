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

function outerLazyOnly(mode: number, bias: number): number {
  let acc = bias;
  try {
    acc += 3;
    try {
      if (mode == 1) {
        throw acc + 10;
      }
      acc += 20;
    } catch (e) {
      acc += 300;
      return acc + (e as number);
    }
    if (mode == 2) {
      throw acc + 23;
    }
    acc += 5;
  } catch (e) {
    return 1500 + acc + (e as number);
  }
  return acc;
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  outerLazyOnly(1, 7);
}

print(outerLazyOnly(1, 7));
ArkTools.arkSteedCompileSync(outerLazyOnly);
print(outerLazyOnly(2, 7));
