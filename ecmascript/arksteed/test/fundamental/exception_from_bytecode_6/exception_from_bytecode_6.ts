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

function exception_from_bytecode_6(a: number, b: number) {
  return a - b;  // JIT -> INT
}

ArkTools.arkSteedCompileSync(exception_from_bytecode_6);

let catchFlag = 0;
try {
  print(`Case 1: Result = ${exception_from_bytecode_6(3, 2)}`);
} catch (e) {
  print(`Case 1: Throws ${(e as Error).name}`);
  catchFlag += 1;
}

try {
  print(`Case 2: Result = ${exception_from_bytecode_6(3n as any, 2)}`)
} catch (e) {
  print(`Case 2: Throws ${(e as Error).name}`);
  catchFlag += 2;
}
print(`catchFlag = ${catchFlag}`)
