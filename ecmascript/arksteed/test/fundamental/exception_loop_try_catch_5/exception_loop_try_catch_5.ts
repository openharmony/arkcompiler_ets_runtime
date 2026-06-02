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

// Triple nested for loop with innermost try-catch
function exception_loop_try_catch_5(n: number, m: number): number {
  let r: number = 0;
  for (let i: number = 0; i < n; i++) {
    for (let j: number = 0; j < m; j++) {
      for (let k: number = 0; k < 2; k++) {
        try {
          let v: number = i * 100 + j * 10 + k;
          if (v % 7 === 0) {
            throw new Error("div7");
          }
          r += 1;
        } catch (e) {
          r += 2;
        }
      }
    }
  }
  return r;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_5);
print(exception_loop_try_catch_5(0, 0));
print(exception_loop_try_catch_5(1, 2));
print(exception_loop_try_catch_5(2, 3));
print(exception_loop_try_catch_5(3, 3));
