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

// finally variant of exception_from_bytecode_6: array access with try-catch-finally
function exception_from_bytecode_finally_3(a: number[], index: number): number {
  let result: number = 0;
  try { result = a[index]; } catch (e) { result = -1; } finally { result += 100; }
  return result;
}

ArkTools.arkSteedCompileSync(exception_from_bytecode_finally_3);
print(exception_from_bytecode_finally_3([10, 20, 30], 1));
print(exception_from_bytecode_finally_3(null as any, 0));
