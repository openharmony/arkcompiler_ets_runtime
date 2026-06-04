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

// Do-while loop with try-catch, throw on multiples of 3
function exception_loop_try_catch_3(n: number): number {
  let r: number = 0;
  let i: number = 0;
  do {
    try {
      r += i * i;
      if (i > 0 && i % 3 === 0) {
        throw new Error("multiple of 3");
      }
      r += i;
    } catch (e) {
      r += 5;
    }
    i++;
  } while (i < n);
  return r;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_3);
print(exception_loop_try_catch_3(0));
print(exception_loop_try_catch_3(3));
print(exception_loop_try_catch_3(6));
print(exception_loop_try_catch_3(9));
