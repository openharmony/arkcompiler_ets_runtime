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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/typeParameterAsTypeParameterConstraint.ts ===
declare function AssertType(value:any, type:string):void;
// using a type parameter as a constraint for a type parameter is valid
// no errors expected except illegal constraints

function foo<T, U extends T>(x: T, y: U): U { 
AssertType(y, "U");
return y; 
}

let r = foo(1, 2);
AssertType(r, "number");
AssertType(foo(1, 2), "int");
AssertType(foo, "<T, U extends T>(T, U) => U");
AssertType(1, "int");
AssertType(2, "int");

let r = foo({}, 1);
AssertType(r, "number");
AssertType(foo({}, 1), "int");
AssertType(foo, "<T, U extends T>(T, U) => U");
AssertType({}, "{}");
AssertType(1, "int");

interface A {
    foo: string;
}
interface B extends A {
    bar: number;
}
let a: A;
AssertType(a, "A");

let b: B;
AssertType(b, "B");

let r2 = foo(a, b);
AssertType(r2, "B");
AssertType(foo(a, b), "B");
AssertType(foo, "<T, U extends T>(T, U) => U");
AssertType(a, "A");
AssertType(b, "B");

let r3 = foo({ x: 1 }, { x: 2, y: 3 });
AssertType(r3, "{ x: number; y: number; }");
AssertType(foo({ x: 1 }, { x: 2, y: 3 }), "{ x: number; y: number; }");
AssertType(foo, "<T, U extends T>(T, U) => U");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType({ x: 2, y: 3 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(2, "int");
AssertType(y, "number");
AssertType(3, "int");

function foo2<T, U extends { length: T }>(x: T, y: U) { 
AssertType(y, "U");
return y; 
}

foo2(1, '');
AssertType(foo2(1, ''), "string");
AssertType(foo2, "<T, U extends { length: T; }>(T, U) => U");
AssertType(1, "int");
AssertType('', "string");

foo2({}, { length: 2 }); 
AssertType(foo2({}, { length: 2 }), "{ length: number; }");
AssertType(foo2, "<T, U extends { length: T; }>(T, U) => U");
AssertType({}, "{}");
AssertType({ length: 2 }, "{ length: number; }");
AssertType(length, "number");
AssertType(2, "int");

foo2(1, { width: 3, length: 2 }); 
AssertType(foo2(1, { width: 3, length: 2 }), "{ width: number; length: number; }");
AssertType(foo2, "<T, U extends { length: T; }>(T, U) => U");
AssertType(1, "int");
AssertType({ width: 3, length: 2 }, "{ width: number; length: number; }");
AssertType(width, "number");
AssertType(3, "int");
AssertType(length, "number");
AssertType(2, "int");

foo2(1, []);
AssertType(foo2(1, []), "any[]");
AssertType(foo2, "<T, U extends { length: T; }>(T, U) => U");
AssertType(1, "int");
AssertType([], "undefined[]");

foo2(1, ['']);
AssertType(foo2(1, ['']), "string[]");
AssertType(foo2, "<T, U extends { length: T; }>(T, U) => U");
AssertType(1, "int");
AssertType([''], "string[]");
AssertType('', "string");


