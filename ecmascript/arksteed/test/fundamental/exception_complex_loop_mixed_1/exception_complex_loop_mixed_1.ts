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


// Complex mixed scenario: loops + nested try-catch-finally + accumulator
function exception_complex_loop_mixed_1(n: number): number {
  let a: number = 0;
  let b: number = 0;
  for (let i: number = 0; i < n; i++) {
    try {
      a += 1;
      try {
        a += 2;
        if (i % 2 === 1) {
          throw new Error("inner throw");
        }
        a += 3;
      } catch (e) {
        a += 10;
        b += 1;
      } finally {
        a += 4;
      }
      a += 5;
      if (i % 3 === 2) {
        throw new Error("outer throw");
      }
    } catch (e) {
      a += 20;
      b += 2;
    } finally {
      a += 6;
    }
  }
  return a * 100 + b;
}

ArkTools.arkSteedCompileSync(exception_complex_loop_mixed_1);

print(exception_complex_loop_mixed_1(0));
print(exception_complex_loop_mixed_1(3));
print(exception_complex_loop_mixed_1(6));

