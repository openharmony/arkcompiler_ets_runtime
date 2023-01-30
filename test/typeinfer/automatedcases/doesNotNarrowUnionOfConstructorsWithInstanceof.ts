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

// === tests/cases/compiler/doesNotNarrowUnionOfConstructorsWithInstanceof.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    length: 1
    constructor() {
      this.length = 1
AssertType(this.length = 1, "int");
AssertType(this.length, "int");
AssertType(this, "this");
AssertType(1, "int");
    }
}

class B {
    length: 2
    constructor() {
      this.length = 2
AssertType(this.length = 2, "int");
AssertType(this.length, "int");
AssertType(this, "this");
AssertType(2, "int");
    }
}

function getTypedArray(flag: boolean) {
AssertType(flag ? new A() : new B(), "union");
AssertType(flag, "boolean");
AssertType(new A(), "A");
AssertType(A, "typeof A");
AssertType(new B(), "B");
AssertType(B, "typeof B");
  return flag ? new A() : new B();
}
function getTypedArrayConstructor(flag: boolean) {
AssertType(flag ? A : B, "union");
AssertType(flag, "boolean");
AssertType(A, "typeof A");
AssertType(B, "typeof B");
  return flag ? A : B;
}
const a = getTypedArray(true);              // A | B
AssertType(a, "union");
AssertType(getTypedArray(true), "union");
AssertType(getTypedArray, "(boolean) => union");
AssertType(true, "boolean");

const b = getTypedArrayConstructor(false);  // A constructor | B constructor
AssertType(b, "union");
AssertType(getTypedArrayConstructor(false), "union");
AssertType(getTypedArrayConstructor, "(boolean) => union");
AssertType(false, "boolean");

if (!(a instanceof b)) {
  console.log(a.length);  // Used to be property 'length' does not exist on type 'never'.
AssertType(console.log(a.length), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(a.length, "union");
}


