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

// === tests/cases/compiler/commentsArgumentsOfCallExpression2.ts ===
declare function AssertType(value:any, type:string):void;
function foo(/*c1*/ x: any, /*d1*/ y: any,/*e1*/w?: any) { }
let a, b: any;
AssertType(a, "any");
AssertType(b, "any");

foo(/*c2*/ 1, /*d2*/ 1 + 2, /*e1*/ a + b);
AssertType(foo(/*c2*/ 1, /*d2*/ 1 + 2, /*e1*/ a + b), "void");
AssertType(foo, "(any, any, ?any) => void");
AssertType(1, "int");
AssertType(1 + 2, "number");
AssertType(1, "int");
AssertType(2, "int");
AssertType(a + b, "any");
AssertType(a, "any");
AssertType(b, "any");

foo(/*c3*/ function () { }, /*d2*/() => { }, /*e2*/ a + /*e3*/ b);
AssertType(foo(/*c3*/ function () { }, /*d2*/() => { }, /*e2*/ a + /*e3*/ b), "void");
AssertType(foo, "(any, any, ?any) => void");
AssertType(function () { }, "() => void");
AssertType(() => { }, "() => void");
AssertType(a + /*e3*/ b, "any");
AssertType(a, "any");
AssertType(b, "any");

foo(/*c3*/ function () { }, /*d3*/() => { }, /*e3*/(a + b));
AssertType(foo(/*c3*/ function () { }, /*d3*/() => { }, /*e3*/(a + b)), "void");
AssertType(foo, "(any, any, ?any) => void");
AssertType(function () { }, "() => void");
AssertType(() => { }, "() => void");
AssertType((a + b), "any");
AssertType(a + b, "any");
AssertType(a, "any");
AssertType(b, "any");

foo(
AssertType(foo(    /*c4*/ function () { },    /*d4*/() => { },    /*e4*/    /*e5*/ "hello"), "void");
AssertType(foo, "(any, any, ?any) => void");

    /*c4*/ function () { },
AssertType(function () { }, "() => void");

    /*d4*/() => { },
AssertType(() => { }, "() => void");

    /*e4*/
    /*e5*/ "hello");
AssertType("hello", "string");


