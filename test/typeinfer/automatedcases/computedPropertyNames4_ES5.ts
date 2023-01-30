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

// === tests/cases/conformance/es6/computedProperties/computedPropertyNames4_ES5.ts ===
declare function AssertType(value:any, type:string):void;
let s: string;
AssertType(s, "string");

let n: number;
AssertType(n, "number");

let a: any;
AssertType(a, "any");

let v = {
AssertType(v, "{ [string]: union; [number]: union; "": number; 0: number; "hello bye": number; }");
AssertType({    [s]: 0,    [n]: n,    [s + s]: 1,    [s + n]: 2,    [+s]: s,    [""]: 0,    [0]: 0,    [a]: 1,    [<any>true]: 0,    [`hello bye`]: 0,    [`hello ${a} bye`]: 0}, "{ [string]: union; [number]: union; "": number; 0: number; "hello bye": number; }");

    [s]: 0,
AssertType([s], "number");
AssertType(s, "string");
AssertType(0, "int");

    [n]: n,
AssertType([n], "number");
AssertType(n, "number");
AssertType(n, "number");

    [s + s]: 1,
AssertType([s + s], "number");
AssertType(s + s, "string");
AssertType(s, "string");
AssertType(s, "string");
AssertType(1, "int");

    [s + n]: 2,
AssertType([s + n], "number");
AssertType(s + n, "string");
AssertType(s, "string");
AssertType(n, "number");
AssertType(2, "int");

    [+s]: s,
AssertType([+s], "string");
AssertType(+s, "number");
AssertType(s, "string");
AssertType(s, "string");

    [""]: 0,
AssertType([""], "number");
AssertType("", "string");
AssertType(0, "int");

    [0]: 0,
AssertType([0], "number");
AssertType(0, "int");
AssertType(0, "int");

    [a]: 1,
AssertType([a], "number");
AssertType(a, "any");
AssertType(1, "int");

    [<any>true]: 0,
AssertType([<any>true], "number");
AssertType(<any>true, "any");
AssertType(true, "boolean");
AssertType(0, "int");

    [`hello bye`]: 0,
AssertType([`hello bye`], "number");
AssertType(`hello bye`, "string");
AssertType(0, "int");

    [`hello ${a} bye`]: 0
AssertType([`hello ${a} bye`], "number");
AssertType(`hello ${a} bye`, "string");
AssertType(a, "any");
AssertType(0, "int");
}

