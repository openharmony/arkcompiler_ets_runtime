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

// Try-catch inside for loop: some iterations throw, caught and continue
function exception_loop_try_catch_1(n: number, k: number): number {
  let result: number = 0;
  for (let i: number = 0; i < n; i++) {
    try {
      if (i === k) {
        throw new Error("skip");
      }
      result += i * 2 + 1;
    } catch (e) {
      result += 1;
    }
  }
  return result;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_1);

// n=0: no iterations → 0
print(exception_loop_try_catch_1(0, 0));
// n=3, k=0: throws at i=0 → result = 1(catch) + (1*2+1)+(2*2+1) = 1+3+5 = 9
print(exception_loop_try_catch_1(3, 0));
// n=5, k=2: throws at i=2 → result = 1+3+1(catch)+7+9 = 21
print(exception_loop_try_catch_1(5, 2));
// n=5, k=10: no throw → result = 1+3+5+7+9 = 25
print(exception_loop_try_catch_1(5, 10));
// n=6, k=3: throws at i=3 → result = 1+3+5+1(catch)+9+11 = 30
print(exception_loop_try_catch_1(6, 3));
