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

// === tests/cases/conformance/es6/spread/arrayLiteralSpread.ts ===
declare function AssertType(value:any, type:string):void;
function f0() {
    let a = [1, 2, 3];
AssertType(a, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

    let a1 = [...a];
AssertType(a1, "number[]");
AssertType([...a], "number[]");
AssertType(...a, "number");
AssertType(a, "number[]");

    let a2 = [1, ...a];
AssertType(a2, "number[]");
AssertType([1, ...a], "number[]");
AssertType(1, "int");
AssertType(...a, "number");
AssertType(a, "number[]");

    let a3 = [1, 2, ...a];
AssertType(a3, "number[]");
AssertType([1, 2, ...a], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "number");
AssertType(a, "number[]");

    let a4 = [...a, 1];
AssertType(a4, "number[]");
AssertType([...a, 1], "number[]");
AssertType(...a, "number");
AssertType(a, "number[]");
AssertType(1, "int");

    let a5 = [...a, 1, 2];
AssertType(a5, "number[]");
AssertType([...a, 1, 2], "number[]");
AssertType(...a, "number");
AssertType(a, "number[]");
AssertType(1, "int");
AssertType(2, "int");

    let a6 = [1, 2, ...a, 1, 2];
AssertType(a6, "number[]");
AssertType([1, 2, ...a, 1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "number");
AssertType(a, "number[]");
AssertType(1, "int");
AssertType(2, "int");

    let a7 = [1, ...a, 2, ...a];
AssertType(a7, "number[]");
AssertType([1, ...a, 2, ...a], "number[]");
AssertType(1, "int");
AssertType(...a, "number");
AssertType(a, "number[]");
AssertType(2, "int");
AssertType(...a, "number");
AssertType(a, "number[]");

    let a8 = [...a, ...a, ...a];
AssertType(a8, "number[]");
AssertType([...a, ...a, ...a], "number[]");
AssertType(...a, "number");
AssertType(a, "number[]");
AssertType(...a, "number");
AssertType(a, "number[]");
AssertType(...a, "number");
AssertType(a, "number[]");
}

function f1() {
    let a = [1, 2, 3];
AssertType(a, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

    let b = ["hello", ...a, true];
AssertType(b, "(union)[]");
AssertType(["hello", ...a, true], "(union)[]");
AssertType("hello", "string");
AssertType(...a, "number");
AssertType(a, "number[]");
AssertType(true, "boolean");

    let b: (string | number | boolean)[];
AssertType(b, "(union)[]");
}

function f2() {
    let a = [...[...[...[...[...[]]]]]];
AssertType(a, "any[]");
AssertType([...[...[...[...[...[]]]]]], "undefined[]");
AssertType(...[...[...[...[...[]]]]], "undefined");
AssertType([...[...[...[...[]]]]], "undefined[]");
AssertType(...[...[...[...[]]]], "undefined");
AssertType([...[...[...[]]]], "undefined[]");
AssertType(...[...[...[]]], "undefined");
AssertType([...[...[]]], "undefined[]");
AssertType(...[...[]], "undefined");
AssertType([...[]], "undefined[]");
AssertType(...[], "undefined");
AssertType([], "undefined[]");

    let b = [...[...[...[...[...[5]]]]]];
AssertType(b, "number[]");
AssertType([...[...[...[...[...[5]]]]]], "number[]");
AssertType(...[...[...[...[...[5]]]]], "number");
AssertType([...[...[...[...[5]]]]], "number[]");
AssertType(...[...[...[...[5]]]], "number");
AssertType([...[...[...[5]]]], "number[]");
AssertType(...[...[...[5]]], "number");
AssertType([...[...[5]]], "number[]");
AssertType(...[...[5]], "number");
AssertType([...[5]], "number[]");
AssertType(...[5], "number");
AssertType([5], "number[]");
AssertType(5, "int");
}


