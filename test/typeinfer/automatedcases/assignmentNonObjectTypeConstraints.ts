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

// === tests/cases/compiler/assignmentNonObjectTypeConstraints.ts ===
declare function AssertType(value:any, type:string):void;
const enum E { A, B, C }

function foo<T extends number>(x: T) {
    let y: number = x;  // Ok
AssertType(y, "number");
AssertType(x, "T");
}

foo(5);
AssertType(foo(5), "void");
AssertType(foo, "<T extends number>(T) => void");
AssertType(5, "int");

foo(E.A);
AssertType(foo(E.A), "void");
AssertType(foo, "<T extends number>(T) => void");
AssertType(E.A, "E.A");

class A { a }
class B { b }

function bar<T extends A | B>(x: T) {
    let y: A | B = x;  // Ok
AssertType(y, "union");
AssertType(x, "union");
}

bar(new A);
AssertType(bar(new A), "void");
AssertType(bar, "<T extends union>(T) => void");
AssertType(new A, "A");
AssertType(A, "typeof A");

bar(new B);
AssertType(bar(new B), "void");
AssertType(bar, "<T extends union>(T) => void");
AssertType(new B, "B");
AssertType(B, "typeof B");


