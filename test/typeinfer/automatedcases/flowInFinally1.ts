/*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Copyright (c) 2023 Huawei Device Co., Ltd.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/flowInFinally1.ts ===
declare function AssertType(value:any, type:string):void;
class A {
  constructor() { }
  method() { }
}

let a: A | null = null;
AssertType(a, "union");
AssertType(null, "null");
AssertType(null, "null");

try {
  a = new A();
AssertType(a = new A(), "A");
AssertType(a, "union");
AssertType(new A(), "A");
AssertType(A, "typeof A");

} finally {
  if (a) {
AssertType(a, "union");

    a.method();
AssertType(a.method(), "void");
AssertType(a.method, "() => void");
  }
}

