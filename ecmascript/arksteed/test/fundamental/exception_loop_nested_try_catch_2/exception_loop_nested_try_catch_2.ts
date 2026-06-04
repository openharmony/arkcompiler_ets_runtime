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

// Multi-level nested try-catch inside loop with exception propagation
function exception_loop_nested_try_catch_2(n: number): number {
  let r: number = 0;
  for (let i: number = 0; i < n; i++) {
    try {
      r += 1;
      try {
        r += 2;
        if (i % 4 === 1) {
          throw new Error("L2");
        }
        r += 3;
      } catch (e) {
        r += 10;
        if (i % 4 === 2) {
          throw new Error("L1");
        }
      }
      r += 4;
    } catch (e) {
      r += 20;
    }
  }
  return r;
}

ArkTools.arkSteedCompileSync(exception_loop_nested_try_catch_2);

print(exception_loop_nested_try_catch_2(0));
print(exception_loop_nested_try_catch_2(4));
print(exception_loop_nested_try_catch_2(8));

