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


// Double nested for loop with inner try-catch
function exception_double_loop_try_catch_1(n: number, m: number): number {
  let result: number = 0;
  for (let i: number = 0; i < n; i++) {
    for (let j: number = 0; j < m; j++) {
      try {
        let val: number = i * m + j;
        if (val % 5 === 0) {
          throw new Error("div5");
        }
        result += val;
      } catch (e) {
        result += 1;
      }
    }
  }
  return result;
}

ArkTools.arkSteedCompileSync(exception_double_loop_try_catch_1);

print(exception_double_loop_try_catch_1(0, 0));
print(exception_double_loop_try_catch_1(2, 3));
print(exception_double_loop_try_catch_1(3, 4));

