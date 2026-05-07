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
declare function print(arg: number): string;

declare class ArkTools {
  static arkSteedCompileAsync<T extends Function>(func: T): T;
}

function exception_no_throw_with_finally(a: number, b: number): number {
  let result = 0;
  try {
    if (a < 0 || b < 0) {
      throw new Error("Negative numbers not allowed");
    }
    result = a - b;
  } catch (e) {
    result = -1;
  } finally {
    result += 10;
  }
  return result;
}

ArkTools.arkSteedCompileAsync(exception_no_throw_with_finally);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(exception_no_throw_with_finally(20, 5));
print(exception_no_throw_with_finally(10, 10));
print(exception_no_throw_with_finally(5, 3));
