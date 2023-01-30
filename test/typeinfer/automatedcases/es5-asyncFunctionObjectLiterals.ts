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

// === tests/cases/compiler/es5-asyncFunctionObjectLiterals.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function objectLiteral0() {
    x = {
AssertType(x = {        a: await y,        b: z    }, "{ a: any; b: any; }");
AssertType(x, "any");
AssertType({        a: await y,        b: z    }, "{ a: any; b: any; }");

        a: await y,
AssertType(a, "any");
AssertType(await y, "any");
AssertType(y, "any");

        b: z
AssertType(b, "any");
AssertType(z, "any");

    };
}

async function objectLiteral1() {
    x = {
AssertType(x = {        a: y,        b: await z    }, "{ a: any; b: any; }");
AssertType(x, "any");
AssertType({        a: y,        b: await z    }, "{ a: any; b: any; }");

        a: y,
AssertType(a, "any");
AssertType(y, "any");

        b: await z
AssertType(b, "any");
AssertType(await z, "any");
AssertType(z, "any");

    };
}

async function objectLiteral2() {
    x = {
AssertType(x = {        [await a]: y,        b: z    }, "{ [number]: any; b: any; }");
AssertType(x, "any");
AssertType({        [await a]: y,        b: z    }, "{ [number]: any; b: any; }");

        [await a]: y,
AssertType([await a], "any");
AssertType(await a, "any");
AssertType(a, "any");
AssertType(y, "any");

        b: z
AssertType(b, "any");
AssertType(z, "any");

    };
}

async function objectLiteral3() {
    x = {
AssertType(x = {        [a]: await y,        b: z    }, "{ [number]: any; b: any; }");
AssertType(x, "any");
AssertType({        [a]: await y,        b: z    }, "{ [number]: any; b: any; }");

        [a]: await y,
AssertType([a], "any");
AssertType(a, "any");
AssertType(await y, "any");
AssertType(y, "any");

        b: z
AssertType(b, "any");
AssertType(z, "any");

    };
}

async function objectLiteral4() {
    x = {
AssertType(x = {        a: await y,        [b]: z    }, "{ [number]: any; a: any; }");
AssertType(x, "any");
AssertType({        a: await y,        [b]: z    }, "{ [number]: any; a: any; }");

        a: await y,
AssertType(a, "any");
AssertType(await y, "any");
AssertType(y, "any");

        [b]: z
AssertType([b], "any");
AssertType(b, "any");
AssertType(z, "any");

    };
}

async function objectLiteral5() {
    x = {
AssertType(x = {        a: y,        [await b]: z    }, "{ [number]: any; a: any; }");
AssertType(x, "any");
AssertType({        a: y,        [await b]: z    }, "{ [number]: any; a: any; }");

        a: y,
AssertType(a, "any");
AssertType(y, "any");

        [await b]: z
AssertType([await b], "any");
AssertType(await b, "any");
AssertType(b, "any");
AssertType(z, "any");

    };
}

async function objectLiteral6() {
    x = {
AssertType(x = {        a: y,        [b]: await z    }, "{ [number]: any; a: any; }");
AssertType(x, "any");
AssertType({        a: y,        [b]: await z    }, "{ [number]: any; a: any; }");

        a: y,
AssertType(a, "any");
AssertType(y, "any");

        [b]: await z
AssertType([b], "any");
AssertType(b, "any");
AssertType(await z, "any");
AssertType(z, "any");

    };
}

