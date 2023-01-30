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

// === tests/cases/conformance/types/specifyingTypes/typeLiterals/arrayOfFunctionTypes3.ts ===
declare function AssertType(value:any, type:string):void;
// valid uses of arrays of function types

let x = [() => 1, () => { }];
AssertType(x, "(() => void)[]");
AssertType([() => 1, () => { }], "(() => void)[]");
AssertType(() => 1, "() => number");
AssertType(1, "int");
AssertType(() => { }, "() => void");

let r2 = x[0]();
AssertType(r2, "void");
AssertType(x[0](), "void");
AssertType(x[0], "() => void");
AssertType(x, "(() => void)[]");
AssertType(0, "int");

class C {
    foo: string;
}
let y = [C, C];
AssertType(y, "(typeof C)[]");
AssertType([C, C], "(typeof C)[]");
AssertType(C, "typeof C");
AssertType(C, "typeof C");

let r3 = new y[0]();
AssertType(r3, "C");
AssertType(new y[0](), "C");
AssertType(y[0], "typeof C");
AssertType(y, "(typeof C)[]");
AssertType(0, "int");

let a: { (x: number): number; (x: string): string; };
AssertType(a, "{ (number): number; (string): string; }");
AssertType(x, "number");
AssertType(x, "string");

let b: { (x: number): number; (x: string): string; };
AssertType(b, "{ (number): number; (string): string; }");
AssertType(x, "number");
AssertType(x, "string");

let c: { (x: number): number; (x: any): any; };
AssertType(c, "{ (number): number; (any): any; }");
AssertType(x, "number");
AssertType(x, "any");

let z = [a, b, c];
AssertType(z, "{ (number): number; (any): any; }[]");
AssertType([a, b, c], "{ (number): number; (any): any; }[]");
AssertType(a, "{ (number): number; (string): string; }");
AssertType(b, "{ (number): number; (string): string; }");
AssertType(c, "{ (number): number; (any): any; }");

let r4 = z[0];
AssertType(r4, "{ (number): number; (any): any; }");
AssertType(z[0], "{ (number): number; (any): any; }");
AssertType(z, "{ (number): number; (any): any; }[]");
AssertType(0, "int");

let r5 = r4(''); // any not string
AssertType(r5, "any");
AssertType(r4(''), "any");
AssertType(r4, "{ (number): number; (any): any; }");
AssertType('', "string");

let r5b = r4(1);
AssertType(r5b, "number");
AssertType(r4(1), "number");
AssertType(r4, "{ (number): number; (any): any; }");
AssertType(1, "int");

let a2: { <T>(x: T): number; (x: string): string;};
AssertType(a2, "{ <T>(T): number; (string): string; }");
AssertType(x, "T");
AssertType(x, "string");

let b2: { <T>(x: T): number; (x: string): string; };
AssertType(b2, "{ <T>(T): number; (string): string; }");
AssertType(x, "T");
AssertType(x, "string");

let c2: { (x: number): number; <T>(x: T): any; };
AssertType(c2, "{ (number): number; <T>(T): any; }");
AssertType(x, "number");
AssertType(x, "T");

let z2 = [a2, b2, c2];
AssertType(z2, "{ (number): number; <T>(T): any; }[]");
AssertType([a2, b2, c2], "{ (number): number; <T>(T): any; }[]");
AssertType(a2, "{ <T>(T): number; (string): string; }");
AssertType(b2, "{ <T>(T): number; (string): string; }");
AssertType(c2, "{ (number): number; <T>(T): any; }");

let r6 = z2[0];
AssertType(r6, "{ (number): number; <T>(T): any; }");
AssertType(z2[0], "{ (number): number; <T>(T): any; }");
AssertType(z2, "{ (number): number; <T>(T): any; }[]");
AssertType(0, "int");

let r7 = r6(''); // any not string
AssertType(r7, "any");
AssertType(r6(''), "any");
AssertType(r6, "{ (number): number; <T>(T): any; }");
AssertType('', "string");


