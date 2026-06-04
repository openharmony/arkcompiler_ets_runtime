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

// finally variant of exception_from_bytecode_10: delete with try-catch-finally
function exception_from_bytecode_finally_5(obj: any, prop: any): number {
  let result: number = 0;
  try { result = (delete obj[prop]) ? 1 : 0; } catch (e) { result = -1; } finally { result += 100; }
  return result;
}

let testObj10a = { a: 1, b: 2 };
let testObj10b = { c: 3 };

ArkTools.arkSteedCompileSync(exception_from_bytecode_finally_5);
print(exception_from_bytecode_finally_5(testObj10a, "a"));
print(exception_from_bytecode_finally_5(testObj10a, "b"));
print(exception_from_bytecode_finally_5(testObj10b, "c"));
print(exception_from_bytecode_finally_5(testObj10b, "noSuch"));
print(exception_from_bytecode_finally_5(null as any, "prop"));
print(exception_from_bytecode_finally_5(undefined as any, "prop"));
