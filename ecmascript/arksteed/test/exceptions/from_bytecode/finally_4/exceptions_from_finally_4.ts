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

// finally variant of exception_from_bytecode_8: conditional branches with try-catch-finally
let cA8 = 0;
let cB8 = 0;

function exception_from_bytecode_finally_4(f: boolean, g: boolean, h: boolean): number {
  let result: number = 0;
  try {
    if (f) { cA8 += 10; result += cA8; }
    if (g) { cB8 += 100; result += cB8; }
    if (h) { nonExistentGlobal += 1000; result += nonExistentGlobal; }
  } catch (e) { result += 1; } finally { result += 100; }
  return result;
}

ArkTools.arkSteedCompileSync(exception_from_bytecode_finally_4);
print(exception_from_bytecode_finally_4(false, false, false));
print(exception_from_bytecode_finally_4(false, false, true));
print(exception_from_bytecode_finally_4(false, true, false));
print(exception_from_bytecode_finally_4(false, true, true));
print(exception_from_bytecode_finally_4(true, false, false));
print(exception_from_bytecode_finally_4(true, false, true));
print(exception_from_bytecode_finally_4(true, true, false));
print(exception_from_bytecode_finally_4(true, true, true));
