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


// Do-while loop with try-catch
function exception_dowhile_try_catch_1(n: number): number {
  let result: number = 0;
  let i: number = 0;
  do {
    try {
      result += i * 3;
      if (i === 2) {
        throw new Error("at 2");
      }
    } catch (e) {
      result += 1;
    }
    i++;
  } while (i < n);
  return result;
}

ArkTools.arkSteedCompileSync(exception_dowhile_try_catch_1);

print(exception_dowhile_try_catch_1(0));
print(exception_dowhile_try_catch_1(4));
print(exception_dowhile_try_catch_1(6));

