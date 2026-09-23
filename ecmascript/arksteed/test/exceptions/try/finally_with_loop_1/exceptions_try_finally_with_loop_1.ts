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


// Loop inside try-catch-finally
function exception_try_catch_finally_with_loop_1(n: number, throws: number): number {
  let result: number = 0;
  try {
    for (let i: number = 0; i < n; i++) {
      result += i * 2;
      if (throws > 0 && i === throws) {
        throw new Error("mid-loop");
      }
    }
    result += 1;
  } catch (e) {
    result += 50;
  } finally {
    result += 100;
  }
  return result;
}

ArkTools.arkSteedCompileSync(exception_try_catch_finally_with_loop_1);

print(exception_try_catch_finally_with_loop_1(0, -1));
print(exception_try_catch_finally_with_loop_1(4, -1));
print(exception_try_catch_finally_with_loop_1(4, 2));
print(exception_try_catch_finally_with_loop_1(7, 3));

