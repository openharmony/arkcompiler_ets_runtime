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

// === tests/cases/compiler/capturedLetConstInLoop3.ts ===
declare function AssertType(value:any, type:string):void;
///=========let
declare function use(a: any);
function foo0(x) {
    for (let x of []) {
AssertType(x, "any");
AssertType([], "undefined[]");

        let v = x;
AssertType(v, "any");
AssertType(x, "any");

        (function() { 
AssertType((function() { return x + v }), "() => any");
AssertType(function() { return x + v }, "() => any");
AssertType(x + v, "any");
AssertType(x, "any");
AssertType(v, "any");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => any");
AssertType(() => x + v, "() => any");
AssertType(x + v, "any");
AssertType(x, "any");
AssertType(v, "any");
    }

    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "any");
}

function foo0_1(x) {
    for (let x in []) {
AssertType(x, "string");
AssertType([], "undefined[]");

        let v = x;
AssertType(v, "string");
AssertType(x, "string");

        (function() { 
AssertType((function() { return x + v }), "() => string");
AssertType(function() { return x + v }, "() => string");
AssertType(x + v, "string");
AssertType(x, "string");
AssertType(v, "string");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => string");
AssertType(() => x + v, "() => string");
AssertType(x + v, "string");
AssertType(x, "string");
AssertType(v, "string");
    }

    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "string");
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

        let v = x;
AssertType(v, "number");
AssertType(x, "number");

        (function() { 
AssertType((function() { return x + v }), "() => number");
AssertType(function() { return x + v }, "() => number");
AssertType(x + v, "number");
AssertType(x, "number");
AssertType(v, "number");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => number");
AssertType(() => x + v, "() => number");
AssertType(x + v, "number");
AssertType(x, "number");
AssertType(v, "number");
    }

    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}

function foo2(x) {
    while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        let x = 1;
AssertType(x, "number");
AssertType(1, "int");

        let v = x;
AssertType(v, "number");
AssertType(x, "number");

        (function() { 
AssertType((function() { return x + v }), "() => number");
AssertType(function() { return x + v }, "() => number");
AssertType(x + v, "number");
AssertType(x, "number");
AssertType(v, "number");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => number");
AssertType(() => x + v, "() => number");
AssertType(x + v, "number");
AssertType(x, "number");
AssertType(v, "number");
    }
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}

function foo3(x) {
    do {
        let x;
AssertType(x, "any");

        let v;
AssertType(v, "any");

        (function() { 
AssertType((function() { return x + v }), "() => any");
AssertType(function() { return x + v }, "() => any");
AssertType(x + v, "any");
AssertType(x, "any");
AssertType(v, "any");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => any");
AssertType(() => x + v, "() => any");
AssertType(x + v, "any");
AssertType(x, "any");
AssertType(v, "any");

    } while (1 === 1);
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "any");
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

        let v = y;
AssertType(v, "number");
AssertType(y, "number");

        let x = 1;
AssertType(x, "number");
AssertType(1, "int");

        (function() { 
AssertType((function() { return x + v }), "() => number");
AssertType(function() { return x + v }, "() => number");
AssertType(x + v, "number");
AssertType(x, "number");
AssertType(v, "number");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => number");
AssertType(() => x + v, "() => number");
AssertType(x + v, "number");
AssertType(x, "number");
AssertType(v, "number");
    }
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
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

        let v = x;
AssertType(v, "number");
AssertType(x, "number");

        (function() { 
AssertType((function() { return x + y + v }), "() => number");
AssertType(function() { return x + y + v }, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(v, "number");
return x + y + v });

        (() => x + y + v);
AssertType((() => x + y + v), "() => number");
AssertType(() => x + y + v, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(v, "number");
    }
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}


function foo6(x) {
    while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        let x, y;
AssertType(x, "any");
AssertType(y, "any");

        let v = x;
AssertType(v, "any");
AssertType(x, "any");

        (function() { 
AssertType((function() { return x + y + v }), "() => any");
AssertType(function() { return x + y + v }, "() => any");
AssertType(x + y + v, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(v, "any");
return x + y + v });

        (() => x + y + v);
AssertType((() => x + y + v), "() => any");
AssertType(() => x + y + v, "() => any");
AssertType(x + y + v, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(v, "any");
    }
    
    use(v)
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "any");
}

function foo7(x) {
    do {
        let x, y;
AssertType(x, "any");
AssertType(y, "any");

        let v = x;
AssertType(v, "any");
AssertType(x, "any");

        (function() { 
AssertType((function() { return x + y + v }), "() => any");
AssertType(function() { return x + y + v }, "() => any");
AssertType(x + y + v, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(v, "any");
return x + y + v });

        (() => x + y + v);
AssertType((() => x + y + v), "() => any");
AssertType(() => x + y + v, "() => any");
AssertType(x + y + v, "any");
AssertType(x + y, "any");
AssertType(x, "any");
AssertType(y, "any");
AssertType(v, "any");

    } while (1 === 1);
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "any");
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

        let v = x;
AssertType(v, "number");
AssertType(x, "number");

        (function() { 
AssertType((function() { return x + y + v }), "() => number");
AssertType(function() { return x + y + v }, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(v, "number");
return x + y + v });

        (() => x + y + v);
AssertType((() => x + y + v), "() => number");
AssertType(() => x + y + v, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(v, "number");
    }
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}
//===const
function foo0_c(x) {
    for (const x of []) {
AssertType(x, "any");
AssertType([], "undefined[]");

        let v = x;
AssertType(v, "any");
AssertType(x, "any");

        (function() { 
AssertType((function() { return x + v }), "() => any");
AssertType(function() { return x + v }, "() => any");
AssertType(x + v, "any");
AssertType(x, "any");
AssertType(v, "any");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => any");
AssertType(() => x + v, "() => any");
AssertType(x + v, "any");
AssertType(x, "any");
AssertType(v, "any");
    }

    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "any");
}

function foo0_1_c(x) {
    for (const x in []) {
AssertType(x, "string");
AssertType([], "undefined[]");

        let v = x;
AssertType(v, "string");
AssertType(x, "string");

        (function() { 
AssertType((function() { return x + v }), "() => string");
AssertType(function() { return x + v }, "() => string");
AssertType(x + v, "string");
AssertType(x, "string");
AssertType(v, "string");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => string");
AssertType(() => x + v, "() => string");
AssertType(x + v, "string");
AssertType(x, "string");
AssertType(v, "string");
    }

    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "string");
}

function foo1_c(x) {
    for (const x = 0; x < 1;) {
AssertType(x, "int");
AssertType(0, "int");
AssertType(x < 1, "boolean");
AssertType(x, "int");
AssertType(1, "int");

        let v = x;
AssertType(v, "number");
AssertType(x, "int");

        (function() { 
AssertType((function() { return x + v }), "() => number");
AssertType(function() { return x + v }, "() => number");
AssertType(x + v, "number");
AssertType(x, "int");
AssertType(v, "number");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => number");
AssertType(() => x + v, "() => number");
AssertType(x + v, "number");
AssertType(x, "int");
AssertType(v, "number");
    }

    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}

function foo2_c(x) {
    while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        const x = 1;
AssertType(x, "int");
AssertType(1, "int");

        let v = x;
AssertType(v, "number");
AssertType(x, "int");

        (function() { 
AssertType((function() { return x + v }), "() => number");
AssertType(function() { return x + v }, "() => number");
AssertType(x + v, "number");
AssertType(x, "int");
AssertType(v, "number");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => number");
AssertType(() => x + v, "() => number");
AssertType(x + v, "number");
AssertType(x, "int");
AssertType(v, "number");
    }
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}

function foo3_c(x) {
    do {
        const x = 1;
AssertType(x, "int");
AssertType(1, "int");

        let v;
AssertType(v, "any");

        (function() { 
AssertType((function() { return x + v }), "() => any");
AssertType(function() { return x + v }, "() => any");
AssertType(x + v, "any");
AssertType(x, "int");
AssertType(v, "any");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => any");
AssertType(() => x + v, "() => any");
AssertType(x + v, "any");
AssertType(x, "int");
AssertType(v, "any");

    } while (1 === 1);
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "any");
}

function foo4_c(x) {
    for (const y = 0; y < 1;) {
AssertType(y, "int");
AssertType(0, "int");
AssertType(y < 1, "boolean");
AssertType(y, "int");
AssertType(1, "int");

        let v = y;
AssertType(v, "number");
AssertType(y, "int");

        const x = 1;
AssertType(x, "int");
AssertType(1, "int");

        (function() { 
AssertType((function() { return x + v }), "() => number");
AssertType(function() { return x + v }, "() => number");
AssertType(x + v, "number");
AssertType(x, "int");
AssertType(v, "number");
return x + v });

        (() => x + v);
AssertType((() => x + v), "() => number");
AssertType(() => x + v, "() => number");
AssertType(x + v, "number");
AssertType(x, "int");
AssertType(v, "number");
    }
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
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

        let v = x;
AssertType(v, "number");
AssertType(x, "int");

        (function() { 
AssertType((function() { return x + y + v }), "() => number");
AssertType(function() { return x + y + v }, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v, "number");
return x + y + v });

        (() => x + y + v);
AssertType((() => x + y + v), "() => number");
AssertType(() => x + y + v, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v, "number");
    }
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}


function foo6_c(x) {
    while (1 === 1) {
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");

        const x = 1, y = 1;
AssertType(x, "int");
AssertType(1, "int");
AssertType(y, "int");
AssertType(1, "int");

        let v = x;
AssertType(v, "number");
AssertType(x, "int");

        (function() { 
AssertType((function() { return x + y + v }), "() => number");
AssertType(function() { return x + y + v }, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v, "number");
return x + y + v });

        (() => x + y + v);
AssertType((() => x + y + v), "() => number");
AssertType(() => x + y + v, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v, "number");
    }
    
    use(v)
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}

function foo7_c(x) {
    do {
        const x = 1, y = 1;
AssertType(x, "int");
AssertType(1, "int");
AssertType(y, "int");
AssertType(1, "int");

        let v = x;
AssertType(v, "number");
AssertType(x, "int");

        (function() { 
AssertType((function() { return x + y + v }), "() => number");
AssertType(function() { return x + y + v }, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v, "number");
return x + y + v });

        (() => x + y + v);
AssertType((() => x + y + v), "() => number");
AssertType(() => x + y + v, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v, "number");

    } while (1 === 1);
AssertType(1 === 1, "boolean");
AssertType(1, "int");
AssertType(1, "int");
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
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

        let v = x;
AssertType(v, "number");
AssertType(x, "int");

        (function() { 
AssertType((function() { return x + y + v }), "() => number");
AssertType(function() { return x + y + v }, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v, "number");
return x + y + v });

        (() => x + y + v);
AssertType((() => x + y + v), "() => number");
AssertType(() => x + y + v, "() => number");
AssertType(x + y + v, "number");
AssertType(x + y, "number");
AssertType(x, "int");
AssertType(y, "int");
AssertType(v, "number");
    }
    
    use(v);
AssertType(use(v), "any");
AssertType(use, "(any) => any");
AssertType(v, "number");
}

