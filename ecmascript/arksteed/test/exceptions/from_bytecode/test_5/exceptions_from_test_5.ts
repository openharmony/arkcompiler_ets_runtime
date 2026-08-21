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

// Test instanceof with exception (INSTANCEOF bytecode -> CommonStubCallWithIC)
function exception_from_bytecode_5(obj: any, ctor: any): number {
  try {
    return (obj instanceof ctor) ? 1 : 0;
  } catch (e) {
    return -1;
  }
}

class TestClass9 {}
function TestFunc9() {}

ArkTools.arkSteedCompileSync(exception_from_bytecode_5);

// Normal cases: instanceof with valid constructor
print(exception_from_bytecode_5(new TestClass9(), TestClass9));   // 1 (true)
print(exception_from_bytecode_5({}, Object));                      // 1 (true)
print(exception_from_bytecode_5([], Array));                       // 1 (true)
print(exception_from_bytecode_5(42, Number));                      // 0 (false, 42 is primitive)
// Exception cases: ctor is not callable / not a valid constructor
print(exception_from_bytecode_5({}, 42 as any));                   // -1 (TypeError: 42 is not a constructor)
print(exception_from_bytecode_5({}, "string" as any));             // -1 (TypeError)
print(exception_from_bytecode_5({}, null as any));                 // -1 (TypeError)
print(exception_from_bytecode_5({}, undefined as any));            // -1 (TypeError)
