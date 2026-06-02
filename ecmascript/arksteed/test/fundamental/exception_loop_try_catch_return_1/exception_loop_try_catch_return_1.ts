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


// Try-catch with return inside for loop
function exception_loop_try_catch_return_1(n: number): number {
  for (let i: number = 0; i < n; i++) {
    try {
      if (i * i > n * 2) {
        return i;
      }
    } catch (e) {
      // ignore
    }
  }
  return -1;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_return_1);

print(exception_loop_try_catch_return_1(0));
print(exception_loop_try_catch_return_1(10));
print(exception_loop_try_catch_return_1(20));
print(exception_loop_try_catch_return_1(50));

