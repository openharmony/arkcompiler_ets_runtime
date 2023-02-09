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

// === tests/cases/compiler/es5-yieldFunctionObjectLiterals.ts ===
declare function AssertType(value:any, type:string):void;
// mainly to verify indentation of emitted code

function g() { 
AssertType("g", "string");
return "g"; 
}

function* objectLiteral1() {
    const x = {
AssertType(x, "{ a: number; b: any; c: number; }");
AssertType({        a: 1,        b: yield 2,        c: 3,    }, "{ a: number; b: any; c: number; }");

        a: 1,
AssertType(a, "number");
AssertType(1, "int");

        b: yield 2,
AssertType(b, "any");
AssertType(yield 2, "any");
AssertType(2, "int");

        c: 3,
AssertType(c, "number");
AssertType(3, "int");
    }
}

function* objectLiteral2() {
    const x = {
AssertType(x, "{ [string]: any; a: number; c: number; }");
AssertType({        a: 1,        [g()]: yield 2,        c: 3,    }, "{ [string]: any; a: number; c: number; }");

        a: 1,
AssertType(a, "number");
AssertType(1, "int");

        [g()]: yield 2,
AssertType([g()], "any");
AssertType(g(), "string");
AssertType(g, "() => string");
AssertType(yield 2, "any");
AssertType(2, "int");

        c: 3,
AssertType(c, "number");
AssertType(3, "int");
    }
}

function* objectLiteral3() {
    const x = {
AssertType(x, "{ [string]: any; a: number; b: any; c: number; }");
AssertType({        a: 1,        b: yield 2,        [g()]: 3,        c: 4,    }, "{ [string]: any; a: number; b: any; c: number; }");

        a: 1,
AssertType(a, "number");
AssertType(1, "int");

        b: yield 2,
AssertType(b, "any");
AssertType(yield 2, "any");
AssertType(2, "int");

        [g()]: 3,
AssertType([g()], "number");
AssertType(g(), "string");
AssertType(g, "() => string");
AssertType(3, "int");

        c: 4,
AssertType(c, "number");
AssertType(4, "int");
    }
}

function* objectLiteral4() {
    const x = {
AssertType(x, "{ [string]: any; a: number; b: any; c: number; }");
AssertType({        a: 1,        [g()]: 2,        b: yield 3,        c: 4,    }, "{ [string]: any; a: number; b: any; c: number; }");

        a: 1,
AssertType(a, "number");
AssertType(1, "int");

        [g()]: 2,
AssertType([g()], "number");
AssertType(g(), "string");
AssertType(g, "() => string");
AssertType(2, "int");

        b: yield 3,
AssertType(b, "any");
AssertType(yield 3, "any");
AssertType(3, "int");

        c: 4,
AssertType(c, "number");
AssertType(4, "int");
    }
}

function* objectLiteral5() {
    const x = {
AssertType(x, "{ [string]: any; a: number; c: number; }");
AssertType({        a: 1,        [g()]: yield 2,        c: 4,    }, "{ [string]: any; a: number; c: number; }");

        a: 1,
AssertType(a, "number");
AssertType(1, "int");

        [g()]: yield 2,
AssertType([g()], "any");
AssertType(g(), "string");
AssertType(g, "() => string");
AssertType(yield 2, "any");
AssertType(2, "int");

        c: 4,
AssertType(c, "number");
AssertType(4, "int");
    }
}

function* objectLiteral6() {
    const x = {
AssertType(x, "{ [number]: number; a: number; c: number; }");
AssertType({        a: 1,        [yield]: 2,        c: 4,    }, "{ [number]: number; a: number; c: number; }");

        a: 1,
AssertType(a, "number");
AssertType(1, "int");

        [yield]: 2,
AssertType([yield], "number");
AssertType(yield, "any");
AssertType(2, "int");

        c: 4,
AssertType(c, "number");
AssertType(4, "int");
    }
}

function* objectLiteral7() {
    const x = {
AssertType(x, "{ [number]: any; a: number; c: number; }");
AssertType({        a: 1,        [yield]: yield 2,        c: 4,    }, "{ [number]: any; a: number; c: number; }");

        a: 1,
AssertType(a, "number");
AssertType(1, "int");

        [yield]: yield 2,
AssertType([yield], "any");
AssertType(yield, "any");
AssertType(yield 2, "any");
AssertType(2, "int");

        c: 4,
AssertType(c, "number");
AssertType(4, "int");
    }
}


