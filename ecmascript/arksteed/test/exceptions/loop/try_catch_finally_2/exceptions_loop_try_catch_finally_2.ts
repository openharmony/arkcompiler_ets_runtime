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

// While loop with try-catch-finally
function exception_loop_try_catch_finally_2(n: number): number {
  let r: number = 0;
  let i: number = 0;
  while (i < n) {
    try {
      r += i * i;
      if (i % 3 === 1) {
        throw new Error("mod3=1");
      }
      r += i;
    } catch (e) {
      r += 1;
    } finally {
      r += 2;
    }
    i++;
  }
  return r;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_finally_2);
print(exception_loop_try_catch_finally_2(0));
print(exception_loop_try_catch_finally_2(3));
print(exception_loop_try_catch_finally_2(6));
print(exception_loop_try_catch_finally_2(9));
