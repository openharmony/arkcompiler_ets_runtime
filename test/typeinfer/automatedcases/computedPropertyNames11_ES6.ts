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

// === tests/cases/conformance/es6/computedProperties/computedPropertyNames11_ES6.ts ===
declare function AssertType(value:any, type:string):void;
let s: string;
AssertType(s, "string");

let n: number;
AssertType(n, "number");

let a: any;
AssertType(a, "any");

let v = {
AssertType(v, "{ [string]: any; [number]: any; "": any; readonly 0: number; "hello bye": any; }");
AssertType({    get [s]() { return 0; },    set [n](v) { },    get [s + s]() { return 0; },    set [s + n](v) { },    get [+s]() { return 0; },    set [""](v) { },    get [0]() { return 0; },    set [a](v) { },    get [<any>true]() { return 0; },    set [`hello bye`](v) { },    get [`hello ${a} bye`]() { return 0; }}, "{ [string]: any; [number]: any; "": any; readonly 0: number; "hello bye": any; }");

    get [s]() { 
AssertType([s], "number");
AssertType(s, "string");
AssertType(0, "int");
return 0; },

    set [n](v) { },
AssertType([n], "any");
AssertType(n, "number");
AssertType(v, "any");

    get [s + s]() { 
AssertType([s + s], "number");
AssertType(s + s, "string");
AssertType(s, "string");
AssertType(s, "string");
AssertType(0, "int");
return 0; },

    set [s + n](v) { },
AssertType([s + n], "any");
AssertType(s + n, "string");
AssertType(s, "string");
AssertType(n, "number");
AssertType(v, "any");

    get [+s]() { 
AssertType([+s], "number");
AssertType(+s, "number");
AssertType(s, "string");
AssertType(0, "int");
return 0; },

    set [""](v) { },
AssertType([""], "any");
AssertType("", "string");
AssertType(v, "any");

    get [0]() { 
AssertType([0], "number");
AssertType(0, "int");
AssertType(0, "int");
return 0; },

    set [a](v) { },
AssertType([a], "any");
AssertType(a, "any");
AssertType(v, "any");

    get [<any>true]() { 
AssertType([<any>true], "number");
AssertType(<any>true, "any");
AssertType(true, "boolean");
AssertType(0, "int");
return 0; },

    set [`hello bye`](v) { },
AssertType([`hello bye`], "any");
AssertType(`hello bye`, "string");
AssertType(v, "any");

    get [`hello ${a} bye`]() { 
AssertType([`hello ${a} bye`], "number");
return 0; 

AssertType(`hello ${a} bye`, "string");

AssertType(a, "any");

AssertType(0, "int");
}
}

