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


// Accumulator pattern: accumulate values in loop, skip errors
function exception_loop_try_catch_accumulate_1(n: number): number {
  let acc: number = 0;
  let skips: number = 0;
  for (let i: number = 0; i < n; i++) {
    try {
      if (i % 7 === 3 || i % 5 === 2) {
        throw new Error("bad index");
      }
      acc += i;
    } catch (e) {
      skips += 1;
    }
  }
  return acc * 100 + skips;
}

ArkTools.arkSteedCompileSync(exception_loop_try_catch_accumulate_1);

print(exception_loop_try_catch_accumulate_1(0));
print(exception_loop_try_catch_accumulate_1(10));
print(exception_loop_try_catch_accumulate_1(20));

