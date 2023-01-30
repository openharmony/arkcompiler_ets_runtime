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

// === tests/cases/compiler/implicitIndexSignatures.ts ===
declare function AssertType(value:any, type:string):void;
type StringMap = { [x: string]: string };

const empty1 = {};
AssertType(empty1, "{}");
AssertType({}, "{}");

let empty2: {};
AssertType(empty2, "{}");

const names1 = { a: "foo", b: "bar" };
AssertType(names1, "{ a: string; b: string; }");
AssertType({ a: "foo", b: "bar" }, "{ a: string; b: string; }");
AssertType(a, "string");
AssertType("foo", "string");
AssertType(b, "string");
AssertType("bar", "string");

let names2: { a: string, b: string };
AssertType(names2, "{ a: string; b: string; }");
AssertType(a, "string");
AssertType(b, "string");

let map: StringMap;
AssertType(map, "StringMap");

map = { x: "xxx", y: "yyy" };
AssertType(map = { x: "xxx", y: "yyy" }, "{ x: string; y: string; }");
AssertType(map, "StringMap");
AssertType({ x: "xxx", y: "yyy" }, "{ x: string; y: string; }");
AssertType(x, "string");
AssertType("xxx", "string");
AssertType(y, "string");
AssertType("yyy", "string");

map = empty1;
AssertType(map = empty1, "{}");
AssertType(map, "StringMap");
AssertType(empty1, "{}");

map = empty2;
AssertType(map = empty2, "{}");
AssertType(map, "StringMap");
AssertType(empty2, "{}");

map = names1;
AssertType(map = names1, "{ a: string; b: string; }");
AssertType(map, "StringMap");
AssertType(names1, "{ a: string; b: string; }");

map = names2;
AssertType(map = names2, "{ a: string; b: string; }");
AssertType(map, "StringMap");
AssertType(names2, "{ a: string; b: string; }");

declare function getStringIndexValue<T>(map: { [x: string]: T }): T;
declare function getNumberIndexValue<T>(map: { [x: number]: T }): T;

function f1() {
    const o1 = { a: 1, b: 2 };
AssertType(o1, "{ a: number; b: number; }");
AssertType({ a: 1, b: 2 }, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "number");
AssertType(2, "int");

    let o2: { a: number, b: number };
AssertType(o2, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(b, "number");

    const v1 = getStringIndexValue(o1);
AssertType(v1, "number");
AssertType(getStringIndexValue(o1), "number");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(o1, "{ a: number; b: number; }");

    const v2 = getStringIndexValue(o2);
AssertType(v2, "number");
AssertType(getStringIndexValue(o2), "number");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(o2, "{ a: number; b: number; }");
}

function f2() {
    const o1 = { a: "1", b: "2" };
AssertType(o1, "{ a: string; b: string; }");
AssertType({ a: "1", b: "2" }, "{ a: string; b: string; }");
AssertType(a, "string");
AssertType("1", "string");
AssertType(b, "string");
AssertType("2", "string");

    let o2: { a: string, b: string };
AssertType(o2, "{ a: string; b: string; }");
AssertType(a, "string");
AssertType(b, "string");

    const v1 = getStringIndexValue(o1);
AssertType(v1, "string");
AssertType(getStringIndexValue(o1), "string");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(o1, "{ a: string; b: string; }");

    const v2 = getStringIndexValue(o2);
AssertType(v2, "string");
AssertType(getStringIndexValue(o2), "string");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(o2, "{ a: string; b: string; }");
}

function f3() {
    const o1 = { a: 1, b: "2" };
AssertType(o1, "{ a: number; b: string; }");
AssertType({ a: 1, b: "2" }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "string");
AssertType("2", "string");

    let o2: { a: number, b: string };
AssertType(o2, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(b, "string");

    const v1 = getStringIndexValue(o1);
AssertType(v1, "union");
AssertType(getStringIndexValue(o1), "union");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(o1, "{ a: number; b: string; }");

    const v2 = getStringIndexValue(o2);
AssertType(v2, "union");
AssertType(getStringIndexValue(o2), "union");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(o2, "{ a: number; b: string; }");
}

function f4() {
    const o1 = { 0: "0", 1: "1", count: 2 };
AssertType(o1, "{ 0: string; 1: string; count: number; }");
AssertType({ 0: "0", 1: "1", count: 2 }, "{ 0: string; 1: string; count: number; }");
AssertType(0, "string");
AssertType("0", "string");
AssertType(1, "string");
AssertType("1", "string");
AssertType(count, "number");
AssertType(2, "int");

    let o2: { 0: string, 1: string, count: number };    
AssertType(o2, "{ 0: string; 1: string; count: number; }");
AssertType(0, "string");
AssertType(1, "string");
AssertType(count, "number");

    const v1 = getStringIndexValue(o1);
AssertType(v1, "union");
AssertType(getStringIndexValue(o1), "union");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(o1, "{ 0: string; 1: string; count: number; }");

    const v2 = getStringIndexValue(o2);
AssertType(v2, "union");
AssertType(getStringIndexValue(o2), "union");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(o2, "{ 0: string; 1: string; count: number; }");

    const v3 = getNumberIndexValue(o1);
AssertType(v3, "string");
AssertType(getNumberIndexValue(o1), "string");
AssertType(getNumberIndexValue, "<T>({ [number]: T; }) => T");
AssertType(o1, "{ 0: string; 1: string; count: number; }");

    const v4 = getNumberIndexValue(o2);
AssertType(v4, "string");
AssertType(getNumberIndexValue(o2), "string");
AssertType(getNumberIndexValue, "<T>({ [number]: T; }) => T");
AssertType(o2, "{ 0: string; 1: string; count: number; }");
}

function f5() {
    enum E1 { A, B 
AssertType(E1, "E1");

AssertType(A, "E1.A");

AssertType(B, "E1.B");
}

    enum E2 { A = "A", B = "B" 
AssertType(E2, "E2");

AssertType(A, "E2.A");

AssertType("A", "string");

AssertType(B, "E2.B");

AssertType("B", "string");
}

    enum E3 { A = 0, B = "B" 
AssertType(E3, "E3");

AssertType(A, "E3.A");

AssertType(0, "int");

AssertType(B, "E3.B");

AssertType("B", "string");
}

    const v1 = getStringIndexValue(E1);
AssertType(v1, "union");
AssertType(getStringIndexValue(E1), "union");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(E1, "typeof E1");

    const v2 = getStringIndexValue(E2);
AssertType(v2, "E2");
AssertType(getStringIndexValue(E2), "E2");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(E2, "typeof E2");

    const v3 = getStringIndexValue(E3);
AssertType(v3, "union");
AssertType(getStringIndexValue(E3), "union");
AssertType(getStringIndexValue, "<T>({ [string]: T; }) => T");
AssertType(E3, "typeof E3");

    const v4 = getNumberIndexValue(E1);
AssertType(v4, "string");
AssertType(getNumberIndexValue(E1), "string");
AssertType(getNumberIndexValue, "<T>({ [number]: T; }) => T");
AssertType(E1, "typeof E1");

    const v5 = getNumberIndexValue(E2);
AssertType(v5, "unknown");
AssertType(getNumberIndexValue(E2), "unknown");
AssertType(getNumberIndexValue, "<T>({ [number]: T; }) => T");
AssertType(E2, "typeof E2");

    const v6 = getNumberIndexValue(E3);
AssertType(v6, "string");
AssertType(getNumberIndexValue(E3), "string");
AssertType(getNumberIndexValue, "<T>({ [number]: T; }) => T");
AssertType(E3, "typeof E3");
}


