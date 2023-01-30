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

// === tests/cases/compiler/controlFlowJavascript.js ===
declare function AssertType(value:any, type:string):void;
let cond = true;
AssertType(cond, "boolean");
AssertType(true, "boolean");

// CFA for 'let' and no initializer
function f1() {
    let x;
AssertType(x, "any");

    if (cond) {
AssertType(cond, "boolean");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
    }
    if (cond) {
AssertType(cond, "boolean");

        x = "hello";
AssertType(x = "hello", "string");
AssertType(x, "any");
AssertType("hello", "string");
    }
    const y = x;  // string | number | undefined
AssertType(y, "union");
AssertType(x, "union");
}

// CFA for 'let' and 'undefined' initializer
function f2() {
    let x = undefined;
AssertType(x, "any");
AssertType(undefined, "undefined");

    if (cond) {
AssertType(cond, "boolean");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
    }
    if (cond) {
AssertType(cond, "boolean");

        x = "hello";
AssertType(x = "hello", "string");
AssertType(x, "any");
AssertType("hello", "string");
    }
    const y = x;  // string | number | undefined
AssertType(y, "union");
AssertType(x, "union");
}

// CFA for 'let' and 'null' initializer
function f3() {
    let x = null;
AssertType(x, "any");
AssertType(null, "null");

    if (cond) {
AssertType(cond, "boolean");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
    }
    if (cond) {
AssertType(cond, "boolean");

        x = "hello";
AssertType(x = "hello", "string");
AssertType(x, "any");
AssertType("hello", "string");
    }
    const y = x;  // string | number | null
AssertType(y, "union");
AssertType(x, "union");
}

// CFA for 'let' with no initializer
function f5() {
    let x;
AssertType(x, "any");

    if (cond) {
AssertType(cond, "boolean");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
    }
    if (cond) {
AssertType(cond, "boolean");

        x = "hello";
AssertType(x = "hello", "string");
AssertType(x, "any");
AssertType("hello", "string");
    }
    const y = x;  // string | number | undefined
AssertType(y, "union");
AssertType(x, "union");
}

// CFA for 'let' with 'undefined' initializer
function f6() {
    let x = undefined;
AssertType(x, "any");
AssertType(undefined, "undefined");

    if (cond) {
AssertType(cond, "boolean");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
    }
    if (cond) {
AssertType(cond, "boolean");

        x = "hello";
AssertType(x = "hello", "string");
AssertType(x, "any");
AssertType("hello", "string");
    }
    const y = x;  // string | number | undefined
AssertType(y, "union");
AssertType(x, "union");
}

// CFA for 'let' with 'null' initializer
function f7() {
    let x = null;
AssertType(x, "any");
AssertType(null, "null");

    if (cond) {
AssertType(cond, "boolean");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
    }
    if (cond) {
AssertType(cond, "boolean");

        x = "hello";
AssertType(x = "hello", "string");
AssertType(x, "any");
AssertType("hello", "string");
    }
    const y = x;  // string | number | null
AssertType(y, "union");
AssertType(x, "union");
}

// No CFA for captured outer letiables
function f9() {
    let x;
AssertType(x, "any");

    if (cond) {
AssertType(cond, "boolean");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
    }
    if (cond) {
AssertType(cond, "boolean");

        x = "hello";
AssertType(x = "hello", "string");
AssertType(x, "any");
AssertType("hello", "string");
    }
    const y = x;  // string | number | undefined
AssertType(y, "union");
AssertType(x, "union");

    function f() {
AssertType(f, "() => void");

        const z = x;  // any
AssertType(z, "any");
AssertType(x, "any");
    }
}

// No CFA for captured outer letiables
function f10() {
    let x;
AssertType(x, "any");

    if (cond) {
AssertType(cond, "boolean");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
    }
    if (cond) {
AssertType(cond, "boolean");

        x = "hello";
AssertType(x = "hello", "string");
AssertType(x, "any");
AssertType("hello", "string");
    }
    const y = x;  // string | number | undefined
AssertType(y, "union");
AssertType(x, "union");

    const f = () => {
AssertType(f, "() => void");
AssertType(() => {        const z = x;  // any    }, "() => void");

        const z = x;  // any
AssertType(z, "any");
AssertType(x, "any");

    };
}


