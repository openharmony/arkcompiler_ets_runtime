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

// For loop with try-catch, conditional continue and throw
function exception_loop_try_catch_4(n: number): number {
  let r: number = 0;
  for (let i: number = 0; i < n; i++) {
    try {
      if (i % 9 === 3) {
        continue;
      }
      r += i;
      if (i % 7 === 4) {
        throw new Error("mod7=4");
      }
      r += 1;
    } catch (e) {
      r += 3;
    }
  }
  return r;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_4);
print(exception_loop_try_catch_4(0));
print(exception_loop_try_catch_4(5));
print(exception_loop_try_catch_4(10));
print(exception_loop_try_catch_4(14));
