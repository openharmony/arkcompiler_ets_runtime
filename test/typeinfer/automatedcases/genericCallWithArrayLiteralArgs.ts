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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithArrayLiteralArgs.ts ===
declare function AssertType(value:any, type:string):void;
function foo<T>(t: T) {
AssertType(t, "T");
    return t;
}

let r = foo([1, 2]); // number[]
AssertType(r, "number[]");
AssertType(foo([1, 2]), "number[]");
AssertType(foo, "<T>(T) => T");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

let r = foo<number[]>([1, 2]); // number[]
AssertType(r, "number[]");
AssertType(foo<number[]>([1, 2]), "number[]");
AssertType(foo, "<T>(T) => T");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

let ra = foo<any[]>([1, 2]); // any[]
AssertType(ra, "any[]");
AssertType(foo<any[]>([1, 2]), "any[]");
AssertType(foo, "<T>(T) => T");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

let r2 = foo([]); // any[]
AssertType(r2, "any[]");
AssertType(foo([]), "any[]");
AssertType(foo, "<T>(T) => T");
AssertType([], "undefined[]");

let r3 = foo<number[]>([]); // number[]
AssertType(r3, "number[]");
AssertType(foo<number[]>([]), "number[]");
AssertType(foo, "<T>(T) => T");
AssertType([], "undefined[]");

let r4 = foo([1, '']); // {}[]
AssertType(r4, "(union)[]");
AssertType(foo([1, '']), "(union)[]");
AssertType(foo, "<T>(T) => T");
AssertType([1, ''], "(union)[]");
AssertType(1, "int");
AssertType('', "string");

let r5 = foo<any[]>([1, '']); // any[]
AssertType(r5, "any[]");
AssertType(foo<any[]>([1, '']), "any[]");
AssertType(foo, "<T>(T) => T");
AssertType([1, ''], "(union)[]");
AssertType(1, "int");
AssertType('', "string");

let r6 = foo<Object[]>([1, '']); // Object[]
AssertType(r6, "Object[]");
AssertType(foo<Object[]>([1, '']), "Object[]");
AssertType(foo, "<T>(T) => T");
AssertType([1, ''], "(union)[]");
AssertType(1, "int");
AssertType('', "string");


