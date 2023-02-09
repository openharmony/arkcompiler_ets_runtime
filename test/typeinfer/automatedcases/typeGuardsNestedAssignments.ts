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

// === tests/cases/conformance/controlFlow/typeGuardsNestedAssignments.ts ===
declare function AssertType(value:any, type:string):void;
class Foo {
    x: string;
}

declare function getFooOrNull(): Foo | null;
declare function getStringOrNumberOrNull(): string | number | null;

function f1() {
    let foo: Foo | null;
AssertType(foo, "union");
AssertType(null, "null");

    if ((foo = getFooOrNull()) !== null) {
AssertType((foo = getFooOrNull()) !== null, "boolean");
AssertType((foo = getFooOrNull()), "union");
AssertType(foo = getFooOrNull(), "union");
AssertType(foo, "union");
AssertType(getFooOrNull(), "union");
AssertType(getFooOrNull, "() => union");
AssertType(null, "null");

        foo;  // Foo
AssertType(foo, "Foo");
    }
}

function f2() {
    let foo1: Foo | null;
AssertType(foo1, "union");
AssertType(null, "null");

    let foo2: Foo | null;
AssertType(foo2, "union");
AssertType(null, "null");

    if ((foo1 = getFooOrNull(), foo2 = foo1) !== null) {
AssertType((foo1 = getFooOrNull(), foo2 = foo1) !== null, "boolean");
AssertType((foo1 = getFooOrNull(), foo2 = foo1), "union");
AssertType(foo1 = getFooOrNull(), foo2 = foo1, "union");
AssertType(foo1 = getFooOrNull(), "union");
AssertType(foo1, "union");
AssertType(getFooOrNull(), "union");
AssertType(getFooOrNull, "() => union");
AssertType(foo2 = foo1, "union");
AssertType(foo2, "union");
AssertType(foo1, "union");
AssertType(null, "null");

        foo1;  // Foo | null
AssertType(foo1, "union");

        foo2;  // Foo
AssertType(foo2, "Foo");
    }
}

function f3() {
    let obj: Object | null;
AssertType(obj, "union");
AssertType(null, "null");

    if ((obj = getFooOrNull()) instanceof Foo) {
AssertType((obj = getFooOrNull()) instanceof Foo, "boolean");
AssertType((obj = getFooOrNull()), "union");
AssertType(obj = getFooOrNull(), "union");
AssertType(obj, "union");
AssertType(getFooOrNull(), "union");
AssertType(getFooOrNull, "() => union");
AssertType(Foo, "typeof Foo");

        obj;
AssertType(obj, "Foo");
    }
}

function f4() {
    let x: string | number | null;
AssertType(x, "union");
AssertType(null, "null");

    if (typeof (x = getStringOrNumberOrNull()) === "number") {
AssertType(typeof (x = getStringOrNumberOrNull()) === "number", "boolean");
AssertType(typeof (x = getStringOrNumberOrNull()), "union");
AssertType((x = getStringOrNumberOrNull()), "union");
AssertType(x = getStringOrNumberOrNull(), "union");
AssertType(x, "union");
AssertType(getStringOrNumberOrNull(), "union");
AssertType(getStringOrNumberOrNull, "() => union");
AssertType("number", "string");

        x;
AssertType(x, "number");
    }
}

// Repro from #8851

const re = /./g
AssertType(re, "RegExp");
AssertType(/./g, "RegExp");

let match: RegExpExecArray | null
AssertType(match, "union");
AssertType(null, "null");

while ((match = re.exec("xxx")) != null) {
    const length = match[1].length + match[2].length
AssertType(length, "number");
AssertType(match[1].length + match[2].length, "number");
AssertType(match[1].length, "number");
AssertType(match[1], "string");
AssertType(match, "RegExpExecArray");
AssertType(1, "int");
AssertType(match[2].length, "number");
AssertType(match[2], "string");
AssertType(match, "RegExpExecArray");
AssertType(2, "int");
}

