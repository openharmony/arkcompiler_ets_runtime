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

// Max complexity: triple-layered try-catch-finally + nested loop + conditional returns
function exception_loop_try_catch_finally_5(n: number): number {
  let r: number = 0;
  for (let i: number = 0; i < n; i++) {
    try {
      r += 1;
      try {
        r += 2;
        if (i % 4 === 1) {
          throw new Error("L2");
        }
        let j: number = 0;
        while (j < 2) {
          try {
            r += i + j;
            if ((i + j) % 5 === 3) {
              throw new Error("L3");
            }
            r += 1;
          } catch (e) {
            r += 2;
          } finally {
            r += 1;
          }
          j++;
        }
        r += 3;
      } catch (e) {
        r += 5;
        if (i > n / 2 && n > 5) {
          return r + 100;
        }
      } finally {
        r += 2;
      }
      r += 4;
      if (i % 7 === 6) {
        throw new Error("L1");
      }
    } catch (e) {
      r += 10;
      if (i === n - 1 && n > 3) {
        return r + 200;
      }
    } finally {
      r += 3;
    }
  }
  return r;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_finally_5);
print(exception_loop_try_catch_finally_5(0));
print(exception_loop_try_catch_finally_5(2));
print(exception_loop_try_catch_finally_5(4));
print(exception_loop_try_catch_finally_5(7));
print(exception_loop_try_catch_finally_5(10));
