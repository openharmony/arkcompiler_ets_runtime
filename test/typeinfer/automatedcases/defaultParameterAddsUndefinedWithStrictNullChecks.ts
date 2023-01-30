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

// === tests/cases/compiler/defaultParameterAddsUndefinedWithStrictNullChecks.ts ===
declare function AssertType(value:any, type:string):void;
function f(addUndefined1 = "J", addUndefined2?: number) {
AssertType(addUndefined1.length + (addUndefined2 || 0), "number");
AssertType(addUndefined1.length, "number");
AssertType((addUndefined2 || 0), "number");
AssertType(addUndefined2 || 0, "number");
AssertType(addUndefined2, "union");
AssertType(0, "int");
    return addUndefined1.length + (addUndefined2 || 0);
}
function g(addUndefined = "J", addDefined: number) {
AssertType(addUndefined.length + addDefined, "number");
AssertType(addUndefined.length, "number");
AssertType(addDefined, "number");
    return addUndefined.length + addDefined;
}
let total = f() + f('a', 1) + f('b') + f(undefined, 2);
AssertType(total, "number");
AssertType(f() + f('a', 1) + f('b') + f(undefined, 2), "number");
AssertType(f() + f('a', 1) + f('b'), "number");
AssertType(f() + f('a', 1), "number");
AssertType(f(), "number");
AssertType(f, "(?string, ?union) => number");
AssertType(f('a', 1), "number");
AssertType(f, "(?string, ?union) => number");
AssertType('a', "string");
AssertType(1, "int");
AssertType(f('b'), "number");
AssertType(f, "(?string, ?union) => number");
AssertType('b', "string");
AssertType(f(undefined, 2), "number");
AssertType(f, "(?string, ?union) => number");
AssertType(undefined, "undefined");
AssertType(2, "int");

total = g('c', 3) + g(undefined, 4);
AssertType(total = g('c', 3) + g(undefined, 4), "number");
AssertType(total, "number");
AssertType(g('c', 3) + g(undefined, 4), "number");
AssertType(g('c', 3), "number");
AssertType(g, "(union, number) => number");
AssertType('c', "string");
AssertType(3, "int");
AssertType(g(undefined, 4), "number");
AssertType(g, "(union, number) => number");
AssertType(undefined, "undefined");
AssertType(4, "int");

function foo1(x: string = "string", b: number) {
    x.length;
AssertType(x.length, "number");
}

function foo2(x = "string", b: number) {
    x.length; // ok, should be string
AssertType(x.length, "number");
}

function foo3(x: string | undefined = "string", b: number) {
    x.length; // ok, should be string
AssertType(x.length, "number");

    x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "union");
AssertType(undefined, "undefined");
}

function foo4(x: string | undefined = undefined, b: number) {
    x; // should be string | undefined
AssertType(x, "union");

    x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "union");
AssertType(undefined, "undefined");
}

type OptionalNullableString = string | null | undefined;
function allowsNull(val: OptionalNullableString = "") {
    val = null;
AssertType(val = null, "null");
AssertType(val, "OptionalNullableString");
AssertType(null, "null");

    val = 'string and null are both ok';
AssertType(val = 'string and null are both ok', "string");
AssertType(val, "OptionalNullableString");
AssertType('string and null are both ok', "string");
}
allowsNull(null); // still allows passing null
AssertType(allowsNull(null), "void");
AssertType(allowsNull, "(?OptionalNullableString) => void");
AssertType(null, "null");



// .d.ts should have `string | undefined` for foo1, foo2, foo3 and foo4
foo1(undefined, 1);
AssertType(foo1(undefined, 1), "void");
AssertType(foo1, "(union, number) => void");
AssertType(undefined, "undefined");
AssertType(1, "int");

foo2(undefined, 1);
AssertType(foo2(undefined, 1), "void");
AssertType(foo2, "(union, number) => void");
AssertType(undefined, "undefined");
AssertType(1, "int");

foo3(undefined, 1);
AssertType(foo3(undefined, 1), "void");
AssertType(foo3, "(union, number) => void");
AssertType(undefined, "undefined");
AssertType(1, "int");

foo4(undefined, 1);
AssertType(foo4(undefined, 1), "void");
AssertType(foo4, "(union, number) => void");
AssertType(undefined, "undefined");
AssertType(1, "int");


function removeUndefinedButNotFalse(x = true) {
    if (x === false) {
AssertType(x === false, "boolean");
AssertType(x, "boolean");
AssertType(false, "boolean");

AssertType(x, "boolean");
        return x;
    }
}

declare const cond: boolean;
AssertType(cond, "boolean");

function removeNothing(y = cond ? true : undefined) {
    if (y !== undefined) {
AssertType(y !== undefined, "boolean");
AssertType(y, "union");
AssertType(undefined, "undefined");

        if (y === false) {
AssertType(y === false, "boolean");
AssertType(y, "boolean");
AssertType(false, "boolean");

AssertType(y, "boolean");
            return y;
        }
    }
AssertType(true, "boolean");
    return true;
}


