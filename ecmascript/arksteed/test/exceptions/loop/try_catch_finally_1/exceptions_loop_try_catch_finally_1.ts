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


// Try-catch-finally inside for loop with conditional throws
function exception_loop_try_catch_finally_1(n: number): number {
  let sum: number = 0;
  for (let i: number = 0; i < n; i++) {
    try {
      sum += i;
      if (i % 2 === 0) {
        throw new Error("even");
      }
      sum += 1;
    } catch (e) {
      sum += 2;
    } finally {
      sum += 5;
    }
  }
  return sum;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_finally_1);

print(exception_loop_try_catch_finally_1(0));
print(exception_loop_try_catch_finally_1(4));
print(exception_loop_try_catch_finally_1(6));

