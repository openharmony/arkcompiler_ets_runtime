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

// Nested try-catch inside for loop
function exception_loop_nested_try_catch_1(n: number): number {
  let result: number = 0;
  for (let i: number = 0; i < n; i++) {
    result += i;
    try {
      try {
        if (i % 3 === 0) {
          throw new Error("inner");
        }
        result += 1;
      } catch (e) {
        result += 3;
        if (i % 2 === 0) {
          throw new Error("outer");
        }
      }
      result += 5;
    } catch (e) {
      result += 7;
    }
  }
  return result;
}

ArkTools.arkSteedCompileSync(exception_loop_nested_try_catch_1);

print(exception_loop_nested_try_catch_1(0));
print(exception_loop_nested_try_catch_1(3));
print(exception_loop_nested_try_catch_1(6));

