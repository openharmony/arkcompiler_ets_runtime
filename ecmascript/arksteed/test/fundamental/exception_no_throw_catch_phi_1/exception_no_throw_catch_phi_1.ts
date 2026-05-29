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

function iterate(a: number, b: number): number {
  return 2 * a - b + 1;
}

function exception_no_throw_catch_phi_1(a: number, b: number): number {
  try {
    a = iterate(a, b);
    b = iterate(a, b);
    a = iterate(b, a);
    b = iterate(b, a);
    a = iterate(a, b);
    b = iterate(a, b);
    a = iterate(b, a);
    b = iterate(b, a);
    return a;
  } catch (e) {
    return a - b;
  }
}

ArkTools.arkSteedCompileAsync(exception_no_throw_catch_phi_1);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(exception_no_throw_catch_phi_1(0, 0));
print(exception_no_throw_catch_phi_1(1, 2));
print(exception_no_throw_catch_phi_1(2, 1));
