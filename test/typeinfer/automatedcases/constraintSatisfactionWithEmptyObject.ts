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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/constraintSatisfactionWithEmptyObject.ts ===
declare function AssertType(value:any, type:string):void;
// valid uses of a basic object constraint, no errors expected

// Object constraint
function foo<T extends Object>(x: T) { }
let r = foo({});
AssertType(r, "void");
AssertType(foo({}), "void");
AssertType(foo, "<T extends Object>(T) => void");
AssertType({}, "{}");

let a = {};
AssertType(a, "{}");
AssertType({}, "{}");

let r = foo({});
AssertType(r, "void");
AssertType(foo({}), "void");
AssertType(foo, "<T extends Object>(T) => void");
AssertType({}, "{}");

class C<T extends Object> {
    constructor(public x: T) { }
}

let r2 = new C({});
AssertType(r2, "C<{}>");
AssertType(new C({}), "C<{}>");
AssertType(C, "typeof C");
AssertType({}, "{}");

interface I<T extends Object> {
    x: T;
}
let i: I<{}>;
AssertType(i, "I<{}>");

// {} constraint
function foo2<T extends {}>(x: T) { }
let r = foo2({});
AssertType(r, "void");
AssertType(foo2({}), "void");
AssertType(foo2, "<T extends {}>(T) => void");
AssertType({}, "{}");

let a = {};
AssertType(a, "{}");
AssertType({}, "{}");

let r = foo2({});
AssertType(r, "void");
AssertType(foo2({}), "void");
AssertType(foo2, "<T extends {}>(T) => void");
AssertType({}, "{}");

class C2<T extends {}> {
    constructor(public x: T) { }
}

let r2 = new C2({});
AssertType(r2, "C<{}>");
AssertType(new C2({}), "C2<{}>");
AssertType(C2, "typeof C2");
AssertType({}, "{}");

interface I2<T extends {}> {
    x: T;
}
let i2: I2<{}>;
AssertType(i2, "I2<{}>");



