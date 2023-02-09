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

// === tests/cases/conformance/controlFlow/constLocalsInFunctionExpressions.ts ===
declare function AssertType(value:any, type:string):void;
declare function getStringOrNumber(): string | number;

function f1() {
    const x = getStringOrNumber();
AssertType(x, "union");
AssertType(getStringOrNumber(), "union");
AssertType(getStringOrNumber, "() => union");

    if (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        const f = () => x.length;
AssertType(f, "() => number");
AssertType(() => x.length, "() => number");
AssertType(x.length, "number");
    }
}

function f2() {
    const x = getStringOrNumber();
AssertType(x, "union");
AssertType(getStringOrNumber(), "union");
AssertType(getStringOrNumber, "() => union");

    if (typeof x !== "string") {
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        return;
    }
    const f = () => x.length;
AssertType(f, "() => number");
AssertType(() => x.length, "() => number");
AssertType(x.length, "number");
}

function f3() {
    const x = getStringOrNumber();
AssertType(x, "union");
AssertType(getStringOrNumber(), "union");
AssertType(getStringOrNumber, "() => union");

    if (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        const f = function() { 
AssertType(f, "() => number");
AssertType(function() { return x.length; }, "() => number");
AssertType(x.length, "number");
return x.length; };
    }
}

function f4() {
    const x = getStringOrNumber();
AssertType(x, "union");
AssertType(getStringOrNumber(), "union");
AssertType(getStringOrNumber, "() => union");

    if (typeof x !== "string") {
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        return;
    }
    const f = function() { 
AssertType(f, "() => number");
AssertType(function() { return x.length; }, "() => number");
AssertType(x.length, "number");
return x.length; };
}

function f5() {
    const x = getStringOrNumber();
AssertType(x, "union");
AssertType(getStringOrNumber(), "union");
AssertType(getStringOrNumber, "() => union");

    if (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        const f = () => () => x.length;
AssertType(f, "() => () => number");
AssertType(() => () => x.length, "() => () => number");
AssertType(() => x.length, "() => number");
AssertType(x.length, "number");
    }
}

