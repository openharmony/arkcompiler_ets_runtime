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

// === tests/cases/conformance/es2017/useObjectValuesAndEntries1.ts ===
declare function AssertType(value:any, type:string):void;
let o = { a: 1, b: 2 };
AssertType(o, "{ a: number; b: number; }");
AssertType({ a: 1, b: 2 }, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "number");
AssertType(2, "int");

for (let x of Object.values(o)) {
    let y = x;
AssertType(y, "number");
AssertType(x, "number");
}

let entries = Object.entries(o);                    // [string, number][]
AssertType(entries, "[string, number][]");
AssertType(Object.entries(o), "[string, number][]");
AssertType(Object.entries, "{ <T>(union): [string, T][]; ({}): [string, any][]; }");
AssertType(o, "{ a: number; b: number; }");

let values = Object.values(o);                      // number[]
AssertType(values, "number[]");
AssertType(Object.values(o), "number[]");
AssertType(Object.values, "{ <T>(union): T[]; ({}): any[]; }");
AssertType(o, "{ a: number; b: number; }");

let entries1 = Object.entries(1);                   // [string, any][]
AssertType(entries1, "[string, any][]");
AssertType(Object.entries(1), "[string, any][]");
AssertType(Object.entries, "{ <T>(union): [string, T][]; ({}): [string, any][]; }");
AssertType(1, "int");

let values1 = Object.values(1);                     // any[]
AssertType(values1, "any[]");
AssertType(Object.values(1), "any[]");
AssertType(Object.values, "{ <T>(union): T[]; ({}): any[]; }");
AssertType(1, "int");

let entries2 = Object.entries({ a: true, b: 2 });   // [string, number|boolean][]
AssertType(entries2, "[string, union][]");
AssertType(Object.entries({ a: true, b: 2 }), "[string, union][]");
AssertType(Object.entries, "{ <T>(union): [string, T][]; ({}): [string, any][]; }");
AssertType({ a: true, b: 2 }, "{ a: true; b: number; }");
AssertType(a, "boolean");
AssertType(true, "boolean");
AssertType(b, "number");
AssertType(2, "int");

let values2 = Object.values({ a: true, b: 2 });     // (number|boolean)[]
AssertType(values2, "(union)[]");
AssertType(Object.values({ a: true, b: 2 }), "(union)[]");
AssertType(Object.values, "{ <T>(union): T[]; ({}): any[]; }");
AssertType({ a: true, b: 2 }, "{ a: true; b: number; }");
AssertType(a, "boolean");
AssertType(true, "boolean");
AssertType(b, "number");
AssertType(2, "int");

let entries3 = Object.entries({});                  // [string, {}][]
AssertType(entries3, "[string, unknown][]");
AssertType(Object.entries({}), "[string, unknown][]");
AssertType(Object.entries, "{ <T>(union): [string, T][]; ({}): [string, any][]; }");
AssertType({}, "{}");

let values3 = Object.values({});                    // {}[]
AssertType(values3, "unknown[]");
AssertType(Object.values({}), "unknown[]");
AssertType(Object.values, "{ <T>(union): T[]; ({}): any[]; }");
AssertType({}, "{}");

let a = ["a", "b", "c"];
AssertType(a, "string[]");
AssertType(["a", "b", "c"], "string[]");
AssertType("a", "string");
AssertType("b", "string");
AssertType("c", "string");

let entries4 = Object.entries(a);                   // [string, string][]
AssertType(entries4, "[string, string][]");
AssertType(Object.entries(a), "[string, string][]");
AssertType(Object.entries, "{ <T>(union): [string, T][]; ({}): [string, any][]; }");
AssertType(a, "string[]");

let values4 = Object.values(a);                     // string[]
AssertType(values4, "string[]");
AssertType(Object.values(a), "string[]");
AssertType(Object.values, "{ <T>(union): T[]; ({}): any[]; }");
AssertType(a, "string[]");

enum E { A, B }
let entries5 = Object.entries(E);                   // [string, any][]
AssertType(entries5, "[string, union][]");
AssertType(Object.entries(E), "[string, union][]");
AssertType(Object.entries, "{ <T>(union): [string, T][]; ({}): [string, any][]; }");
AssertType(E, "typeof E");

let values5 = Object.values(E);                     // any[]
AssertType(values5, "(union)[]");
AssertType(Object.values(E), "(union)[]");
AssertType(Object.values, "{ <T>(union): T[]; ({}): any[]; }");
AssertType(E, "typeof E");

interface I { }
let i: I = {};
AssertType(i, "I");
AssertType({}, "{}");

let entries6 = Object.entries(i);                   // [string, any][]
AssertType(entries6, "[string, any][]");
AssertType(Object.entries(i), "[string, any][]");
AssertType(Object.entries, "{ <T>(union): [string, T][]; ({}): [string, any][]; }");
AssertType(i, "I");

let values6 = Object.values(i);                     // any[]
AssertType(values6, "any[]");
AssertType(Object.values(i), "any[]");
AssertType(Object.values, "{ <T>(union): T[]; ({}): any[]; }");
AssertType(i, "I");


