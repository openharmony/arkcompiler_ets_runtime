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

// === tests/cases/compiler/contextualTypingOfConditionalExpression.ts ===
declare function AssertType(value:any, type:string):void;
let x: (a: number) => void = true ? (a) => a.toExponential() : (b) => b.toFixed();
AssertType(x, "(number) => void");
AssertType(a, "number");
AssertType(true ? (a) => a.toExponential() : (b) => b.toFixed(), "(number) => string");
AssertType(true, "boolean");
AssertType((a) => a.toExponential(), "(number) => string");
AssertType(a, "number");
AssertType(a.toExponential(), "string");
AssertType(a.toExponential, "(?number) => string");
AssertType((b) => b.toFixed(), "(number) => string");
AssertType(b, "number");
AssertType(b.toFixed(), "string");
AssertType(b.toFixed, "(?number) => string");

class A {
    foo: number;
}
class B extends A {
    bar: number;
}
class C extends A {
    baz: number;
}

let x2: (a: A) => void = true ? (a) => a.foo : (b) => b.foo;
AssertType(x2, "(A) => void");
AssertType(a, "A");
AssertType(true ? (a) => a.foo : (b) => b.foo, "(A) => number");
AssertType(true, "boolean");
AssertType((a) => a.foo, "(A) => number");
AssertType(a, "A");
AssertType(a.foo, "number");
AssertType((b) => b.foo, "(A) => number");
AssertType(b, "A");
AssertType(b.foo, "number");


