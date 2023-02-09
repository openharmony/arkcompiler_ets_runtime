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

// === tests/cases/conformance/types/contextualTypes/methodDeclarations/contextuallyTypedObjectLiteralMethodDeclaration01.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    numProp: number;
}

interface B  {
    strProp: string;
}

interface Foo {
    method1(arg: A): void;
    method2(arg: B): void;
}

function getFoo1(): Foo {
AssertType({        method1(arg) {            arg.numProp = 10;        },        method2(arg) {            arg.strProp = "hello";        }    }, "{ method1(A): void; method2(B): void; }");
    return {

        method1(arg) {
AssertType(method1, "(A) => void");
AssertType(arg, "A");

            arg.numProp = 10;
AssertType(arg.numProp = 10, "int");
AssertType(arg.numProp, "number");
AssertType(10, "int");

        },
        method2(arg) {
AssertType(method2, "(B) => void");
AssertType(arg, "B");

            arg.strProp = "hello";
AssertType(arg.strProp = "hello", "string");
AssertType(arg.strProp, "string");
AssertType("hello", "string");
        }
    }
}

function getFoo2(): Foo {
AssertType({        method1: (arg) => {            arg.numProp = 10;        },        method2: (arg) => {            arg.strProp = "hello";        }    }, "{ method1: (A) => void; method2: (B) => void; }");
    return {

        method1: (arg) => {
AssertType(method1, "(A) => void");
AssertType((arg) => {            arg.numProp = 10;        }, "(A) => void");
AssertType(arg, "A");

            arg.numProp = 10;
AssertType(arg.numProp = 10, "int");
AssertType(arg.numProp, "number");
AssertType(10, "int");

        },
        method2: (arg) => {
AssertType(method2, "(B) => void");
AssertType((arg) => {            arg.strProp = "hello";        }, "(B) => void");
AssertType(arg, "B");

            arg.strProp = "hello";
AssertType(arg.strProp = "hello", "string");
AssertType(arg.strProp, "string");
AssertType("hello", "string");
        }
    }
}

function getFoo3(): Foo {
AssertType({        method1: function (arg) {            arg.numProp = 10;        },        method2: function (arg) {            arg.strProp = "hello";        }    }, "{ method1: (A) => void; method2: (B) => void; }");
    return {

        method1: function (arg) {
AssertType(method1, "(A) => void");
AssertType(function (arg) {            arg.numProp = 10;        }, "(A) => void");
AssertType(arg, "A");

            arg.numProp = 10;
AssertType(arg.numProp = 10, "int");
AssertType(arg.numProp, "number");
AssertType(10, "int");

        },
        method2: function (arg) {
AssertType(method2, "(B) => void");
AssertType(function (arg) {            arg.strProp = "hello";        }, "(B) => void");
AssertType(arg, "B");

            arg.strProp = "hello";
AssertType(arg.strProp = "hello", "string");
AssertType(arg.strProp, "string");
AssertType("hello", "string");
        }
    }
}

