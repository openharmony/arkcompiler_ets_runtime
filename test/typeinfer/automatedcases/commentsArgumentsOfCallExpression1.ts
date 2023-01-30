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

// === tests/cases/compiler/commentsArgumentsOfCallExpression1.ts ===
declare function AssertType(value:any, type:string):void;
function foo(/*c1*/ x: any) { }
foo(/*c2*/ 1);
AssertType(foo(/*c2*/ 1), "void");
AssertType(foo, "(any) => void");
AssertType(1, "int");

foo(/*c3*/ function () { });
AssertType(foo(/*c3*/ function () { }), "void");
AssertType(foo, "(any) => void");
AssertType(function () { }, "() => void");

foo(
AssertType(foo(    /*c4*/    () => { }), "void");
AssertType(foo, "(any) => void");

    /*c4*/
    () => { });
AssertType(() => { }, "() => void");

foo(
AssertType(foo(    /*c5*/    /*c6*/    () => { }), "void");
AssertType(foo, "(any) => void");

    /*c5*/
    /*c6*/
    () => { });
AssertType(() => { }, "() => void");

foo(/*c7*/
AssertType(foo(/*c7*/    () => { }), "void");
AssertType(foo, "(any) => void");

    () => { });
AssertType(() => { }, "() => void");

foo(
AssertType(foo(    /*c7*/    /*c8*/() => { }), "void");
AssertType(foo, "(any) => void");

    /*c7*/
    /*c8*/() => { });
AssertType(() => { }, "() => void");


