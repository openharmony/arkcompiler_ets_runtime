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

function expCase(base: any, exponent: any): number {
  let live = 2;
  let result = 8;
  try {
    live = live + 24;
    result = base ** exponent;
    return live + result;
  } catch (e) {
    return 1200 + live + result;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  expCase(4, 2);
}

print(expCase(4, 2));
ArkTools.arkSteedCompileSync(expCase);
print(expCase(4, Symbol("lazy")));
