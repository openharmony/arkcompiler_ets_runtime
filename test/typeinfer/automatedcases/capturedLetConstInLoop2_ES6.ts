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

// === tests/cases/compiler/capturedLetConstInLoop2_ES6.ts ===
declare function AssertType(value:any, type:string):void;
// ========let
function foo0(x) {
    for (let x of []) {
AssertType(x, "any");
AssertType([], "undefined[]");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => any");
AssertType(function() { return x + a }, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => any");
AssertType(() => x + a, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
    }
}

function foo0_1(x) {
    for (let x in []) {
AssertType(x, "string");
AssertType([], "undefined[]");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => string");
AssertType(function() { return x + a }, "() => string");
AssertType(x + a, "string");
AssertType(x, "string");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => string");
AssertType(() => x + a, "() => string");
AssertType(x + a, "string");
AssertType(x, "string");
AssertType(a, "number");
    }
}

function foo1(x) {
    for (let x = 0; x < 1; ++x) {
AssertType(x, "number");
AssertType(0, "int");
AssertType(x < 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");
AssertType(++x, "number");
AssertType(x, "number");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => number");
AssertType(function() { return x + a }, "() => number");
AssertType(x + a, "number");
AssertType(x, "number");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => number");
AssertType(() => x + a, "() => number");
AssertType(x + a, "number");
AssertType(x, "number");
AssertType(a, "number");
    }
}

function foo2(x) {
    while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => any");
AssertType(function() { return x + a }, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => any");
AssertType(() => x + a, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
    }
}

function foo3(x) {
    do {
        let x;
AssertType(x, "any");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => any");
AssertType(function() { return x + a }, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => any");
AssertType(() => x + a, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");

    } while (1 === 1)
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");
}

function foo4(x) {
    for (let y = 0; y < 1; ++y) {
AssertType(y, "number");
AssertType(0, "int");
AssertType(y < 1, "boolean");
AssertType(y, "number");
AssertType(1, "int");
AssertType(++y, "number");
AssertType(y, "number");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        let x = 1;
AssertType(x, "number");
AssertType(1, "int");

        (function() { 
AssertType((function() { return x + a }), "() => number");
AssertType(function() { return x + a }, "() => number");
AssertType(x + a, "number");
AssertType(x, "number");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => number");
AssertType(() => x + a, "() => number");
AssertType(x + a, "number");
AssertType(x, "number");
AssertType(a, "number");
    }
}

function foo5(x) {
    for (let x = 0, y = 1; x < 1; ++x) {
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "number");
AssertType(1, "int");
AssertType(x < 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");
AssertType(++x, "number");
AssertType(x, "number");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + y + a }), "() => number");
AssertType(function() { return x + y + a }, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(a, "number");
return x + y + a });

        (() => x + y + a);
AssertType((() => x + y + a), "() => number");
AssertType(() => x + y + a, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(a, "number");
    }
}


function foo6(x) {
    while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        let x, y;
AssertType(x, "any");
AssertType(y, "any");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + y + a }), "() => any");
AssertType(function() { return x + y + a }, "() => any");
AssertType(x + y + a, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(a, "number");
return x + y + a });

        (() => x + y + a);
AssertType((() => x + y + a), "() => any");
AssertType(() => x + y + a, "() => any");
AssertType(x + y + a, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(a, "number");
    }
}

function foo7(x) {
    do {
        let x, y;
AssertType(x, "any");
AssertType(y, "any");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + y + a }), "() => any");
AssertType(function() { return x + y + a }, "() => any");
AssertType(x + y + a, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(a, "number");
return x + y + a });

        (() => x + y + a);
AssertType((() => x + y + a), "() => any");
AssertType(() => x + y + a, "() => any");
AssertType(x + y + a, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(a, "number");

    } while (1 === 1)
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");
}


function foo8(x) {
    for (let y = 0; y < 1; ++y) {
AssertType(y, "number");
AssertType(0, "int");
AssertType(y < 1, "boolean");
AssertType(y, "number");
AssertType(1, "int");
AssertType(++y, "number");
AssertType(y, "number");

        let x = 1;
AssertType(x, "number");
AssertType(1, "int");

        let a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + y + a }), "() => number");
AssertType(function() { return x + y + a }, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(a, "number");
return x + y + a });

        (() => x + y + a);
AssertType((() => x + y + a), "() => number");
AssertType(() => x + y + a, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(a, "number");
    }
}
///=======const
function foo0_c(x) {
    for (const x of []) {
AssertType(x, "any");
AssertType([], "undefined[]");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => any");
AssertType(function() { return x + a }, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => any");
AssertType(() => x + a, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
    }
}

function foo0_1_c(x) {
    for (const x in []) {
AssertType(x, "string");
AssertType([], "undefined[]");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => string");
AssertType(function() { return x + a }, "() => string");
AssertType(x + a, "string");
AssertType(x, "string");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => string");
AssertType(() => x + a, "() => string");
AssertType(x + a, "string");
AssertType(x, "string");
AssertType(a, "number");
    }
}

function foo1_c(x) {
    for (const x = 0; x < 1;) {
AssertType(x, "int");
AssertType(0, "int");
AssertType(x < 1, "boolean");
AssertType(x, "int");
AssertType(1, "int");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => number");
AssertType(function() { return x + a }, "() => number");
AssertType(x + a, "number");
AssertType(x, "int");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => number");
AssertType(() => x + a, "() => number");
AssertType(x + a, "number");
AssertType(x, "int");
AssertType(a, "number");
    }
}

function foo2_c(x) {
    while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => any");
AssertType(function() { return x + a }, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => any");
AssertType(() => x + a, "() => any");
AssertType(x + a, "any");
AssertType(x, "any");
AssertType(a, "number");
    }
}

function foo3_c(x) {
    do {
        const x = 1;
AssertType(x, "int");
AssertType(1, "int");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + a }), "() => number");
AssertType(function() { return x + a }, "() => number");
AssertType(x + a, "number");
AssertType(x, "int");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => number");
AssertType(() => x + a, "() => number");
AssertType(x + a, "number");
AssertType(x, "int");
AssertType(a, "number");

    } while (1 === 1)
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");
}

function foo4_c(x) {
    for (const y = 0; y < 1;) {
AssertType(y, "int");
AssertType(0, "int");
AssertType(y < 1, "boolean");
AssertType(y, "int");
AssertType(1, "int");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        const x = 1;
AssertType(x, "int");
AssertType(1, "int");

        (function() { 
AssertType((function() { return x + a }), "() => number");
AssertType(function() { return x + a }, "() => number");
AssertType(x + a, "number");
AssertType(x, "int");
AssertType(a, "number");
return x + a });

        (() => x + a);
AssertType((() => x + a), "() => number");
AssertType(() => x + a, "() => number");
AssertType(x + a, "number");
AssertType(x, "int");
AssertType(a, "number");
    }
}

function foo5_c(x) {
    for (const x = 0, y = 1; x < 1;) {
AssertType(x, "int");
AssertType(0, "int");
AssertType(y, "int");
AssertType(1, "int");
AssertType(x < 1, "boolean");
AssertType(x, "int");
AssertType(1, "int");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + y + a }), "() => number");
AssertType(function() { return x + y + a }, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(a, "number");
return x + y + a });

        (() => x + y + a);
AssertType((() => x + y + a), "() => number");
AssertType(() => x + y + a, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(a, "number");
    }
}


function foo6_c(x) {
    while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        const x = 1, y =1 ;
AssertType(x, "int");
AssertType(1, "int");
AssertType(y, "int");
AssertType(1, "int");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + y + a }), "() => number");
AssertType(function() { return x + y + a }, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(a, "number");
return x + y + a });

        (() => x + y + a);
AssertType((() => x + y + a), "() => number");
AssertType(() => x + y + a, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(a, "number");
    }
}

function foo7_c(x) {
    do {
        const x = 1, y = 1;
AssertType(x, "int");
AssertType(1, "int");
AssertType(y, "int");
AssertType(1, "int");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + y + a }), "() => number");
AssertType(function() { return x + y + a }, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(a, "number");
return x + y + a });

        (() => x + y + a);
AssertType((() => x + y + a), "() => number");
AssertType(() => x + y + a, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(a, "number");

    } while (1 === 1)
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");
}


function foo8_c(x) {
    for (const y = 0; y < 1;) {
AssertType(y, "int");
AssertType(0, "int");
AssertType(y < 1, "boolean");
AssertType(y, "int");
AssertType(1, "int");

        const x = 1;
AssertType(x, "int");
AssertType(1, "int");

        const a = arguments.length;
AssertType(a, "number");
AssertType(arguments.length, "number");

        (function() { 
AssertType((function() { return x + y + a }), "() => number");
AssertType(function() { return x + y + a }, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(a, "number");
return x + y + a });

        (() => x + y + a);
AssertType((() => x + y + a), "() => number");
AssertType(() => x + y + a, "() => number");
AssertType(x + y + a, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(a, "number");
    }
}

