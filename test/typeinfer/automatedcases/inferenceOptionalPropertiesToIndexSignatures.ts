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

// === tests/cases/compiler/inferenceOptionalPropertiesToIndexSignatures.ts ===
declare function AssertType(value:any, type:string):void;
declare function foo<T>(obj: { [x: string]: T }): T;

declare const x1: { a: string, b: number };
AssertType(x1, "{ a: string; b: number; }");
AssertType(a, "string");
AssertType(b, "number");

declare const x2: { a: string, b: number | undefined };
AssertType(x2, "{ a: string; b: union; }");
AssertType(a, "string");
AssertType(b, "union");

declare const x3: { a: string, b?: number };
AssertType(x3, "{ a: string; b?: union; }");
AssertType(a, "string");
AssertType(b, "union");

declare const x4: { a: string, b?: number | undefined };
AssertType(x4, "{ a: string; b?: union; }");
AssertType(a, "string");
AssertType(b, "union");

let a1 = foo(x1);  // string | number
AssertType(a1, "union");
AssertType(foo(x1), "union");
AssertType(foo, "<T>({ [string]: T; }) => T");
AssertType(x1, "{ a: string; b: number; }");

let a2 = foo(x2);  // string | number | undefined
AssertType(a2, "union");
AssertType(foo(x2), "union");
AssertType(foo, "<T>({ [string]: T; }) => T");
AssertType(x2, "{ a: string; b: union; }");

let a3 = foo(x3);  // string | number
AssertType(a3, "union");
AssertType(foo(x3), "union");
AssertType(foo, "<T>({ [string]: T; }) => T");
AssertType(x3, "{ a: string; b?: union; }");

let a4 = foo(x4);  // string | number
AssertType(a4, "union");
AssertType(foo(x4), "union");
AssertType(foo, "<T>({ [string]: T; }) => T");
AssertType(x4, "{ a: string; b?: union; }");

// Repro from #43045

const param2 = Math.random() < 0.5 ? 'value2' : null;
AssertType(param2, "union");
AssertType(Math.random() < 0.5 ? 'value2' : null, "union");
AssertType(Math.random() < 0.5, "boolean");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(0.5, "double");
AssertType('value2', "string");
AssertType(null, "null");

const obj = {
AssertType(obj, "{ param2?: union; param1: string; }");
AssertType({    param1: 'value1',    ...(param2 ? {param2} : {})}, "{ param2?: union; param1: string; }");

    param1: 'value1',
AssertType(param1, "string");
AssertType('value1', "string");

    ...(param2 ? {param2} : {})
AssertType((param2 ? {param2} : {}), "union");
AssertType(param2 ? {param2} : {}, "union");
AssertType(param2, "union");
AssertType({param2}, "{ param2: string; }");
AssertType(param2, "string");
AssertType({}, "{}");

};

const query = Object.entries(obj).map(
AssertType(query, "string");
AssertType(Object.entries(obj).map(    ([k, v]) => `${k}=${encodeURIComponent(v)}`).join('&'), "string");
AssertType(Object.entries(obj).map(    ([k, v]) => `${k}=${encodeURIComponent(v)}`).join, "(?union) => string");

    ([k, v]) => `${k}=${encodeURIComponent(v)}`
).join('&');
AssertType('&', "string");


