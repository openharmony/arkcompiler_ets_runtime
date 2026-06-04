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

let cA = 123;
let cB = 456;

function exception_from_bytecode_4(index: number): number {
  let result = 0;
  try {
    if (index == 0) {
      result = cA;
    } else if (index == 1) {
      result = cB;
    } else {
      result = nonExistentGlobal;
    }
  } catch (e) {
    result = -1;
  }
  return result;
}

ArkTools.arkSteedCompileSync(exception_from_bytecode_4);

print(exception_from_bytecode_4(0));
print(exception_from_bytecode_4(1));
print(exception_from_bytecode_4(2));
