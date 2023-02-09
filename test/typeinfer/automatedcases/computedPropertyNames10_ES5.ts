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

// === tests/cases/conformance/es6/computedProperties/computedPropertyNames10_ES5.ts ===
declare function AssertType(value:any, type:string):void;
let s: string;
AssertType(s, "string");

let n: number;
AssertType(n, "number");

let a: any;
AssertType(a, "any");

let v = {
AssertType(v, "{ [string]: () => void; [number]: () => void; ""(): void; 0(): void; "hello bye"(): void; }");
AssertType({    [s]() { },    [n]() { },    [s + s]() { },    [s + n]() { },    [+s]() { },    [""]() { },    [0]() { },    [a]() { },    [<any>true]() { },    [`hello bye`]() { },    [`hello ${a} bye`]() { }}, "{ [string]: () => void; [number]: () => void; ""(): void; 0(): void; "hello bye"(): void; }");

    [s]() { },
AssertType([s], "() => void");
AssertType(s, "string");

    [n]() { },
AssertType([n], "() => void");
AssertType(n, "number");

    [s + s]() { },
AssertType([s + s], "() => void");
AssertType(s + s, "string");
AssertType(s, "string");
AssertType(s, "string");

    [s + n]() { },
AssertType([s + n], "() => void");
AssertType(s + n, "string");
AssertType(s, "string");
AssertType(n, "number");

    [+s]() { },
AssertType([+s], "() => void");
AssertType(+s, "number");
AssertType(s, "string");

    [""]() { },
AssertType([""], "() => void");
AssertType("", "string");

    [0]() { },
AssertType([0], "() => void");
AssertType(0, "int");

    [a]() { },
AssertType([a], "() => void");
AssertType(a, "any");

    [<any>true]() { },
AssertType([<any>true], "() => void");
AssertType(<any>true, "any");
AssertType(true, "boolean");

    [`hello bye`]() { },
AssertType([`hello bye`], "() => void");
AssertType(`hello bye`, "string");

    [`hello ${a} bye`]() { 
AssertType([`hello ${a} bye`], "() => void");

AssertType(`hello ${a} bye`, "string");

AssertType(a, "any");
}
}

