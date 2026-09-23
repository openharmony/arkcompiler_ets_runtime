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


// Multiple loop iterations where finally must always execute
function exception_loop_finally_multi_exit_1(n: number): number {
  let result: number = 0;
  let finallyCount: number = 0;
  for (let i: number = 0; i < n; i++) {
    try {
      result += i;
      if (i % 3 === 1) {
        continue;
      }
      if (i % 3 === 2) {
        throw new Error("throw at " + i);
      }
      result += 10;
    } catch (e) {
      result += 5;
    } finally {
      finallyCount++;
    }
  }
  return result * 1000 + finallyCount;
}

ArkTools.arkSteedCompileSync(exception_loop_finally_multi_exit_1);

print(exception_loop_finally_multi_exit_1(0));
print(exception_loop_finally_multi_exit_1(6));
print(exception_loop_finally_multi_exit_1(9));

