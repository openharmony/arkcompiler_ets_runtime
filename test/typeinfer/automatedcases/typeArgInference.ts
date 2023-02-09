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

// === tests/cases/compiler/typeArgInference.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
    f<T, U>(a1: { a: T; b: U }[], a2: { a: T; b: U }[]): { c: T; d: U };
    g<T, U>(...arg: { a: T; b: U }[][]): { c: T; d: U };
}
let o = { a: 3, b: "test" };
AssertType(o, "{ a: number; b: string; }");
AssertType({ a: 3, b: "test" }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(3, "int");
AssertType(b, "string");
AssertType("test", "string");

let x: I;
AssertType(x, "I");

let t1 = x.f([o], [o]);
AssertType(t1, "{ c: number; d: string; }");
AssertType(x.f([o], [o]), "{ c: number; d: string; }");
AssertType(x.f, "<T, U>({ a: T; b: U; }[], { a: T; b: U; }[]) => { c: T; d: U; }");
AssertType([o], "{ a: number; b: string; }[]");
AssertType(o, "{ a: number; b: string; }");
AssertType([o], "{ a: number; b: string; }[]");
AssertType(o, "{ a: number; b: string; }");

let t1: { c: number; d: string }; 
AssertType(t1, "{ c: number; d: string; }");
AssertType(c, "number");
AssertType(d, "string");

let t2 = x.f<number, string>([o], [o]);
AssertType(t2, "{ c: number; d: string; }");
AssertType(x.f<number, string>([o], [o]), "{ c: number; d: string; }");
AssertType(x.f, "<T, U>({ a: T; b: U; }[], { a: T; b: U; }[]) => { c: T; d: U; }");
AssertType([o], "{ a: number; b: string; }[]");
AssertType(o, "{ a: number; b: string; }");
AssertType([o], "{ a: number; b: string; }[]");
AssertType(o, "{ a: number; b: string; }");

let t2: { c: number; d: string }; 
AssertType(t2, "{ c: number; d: string; }");
AssertType(c, "number");
AssertType(d, "string");

let t3 = x.g([o], [o]);
AssertType(t3, "{ c: number; d: string; }");
AssertType(x.g([o], [o]), "{ c: number; d: string; }");
AssertType(x.g, "<T, U>(...{ a: T; b: U; }[][]) => { c: T; d: U; }");
AssertType([o], "{ a: number; b: string; }[]");
AssertType(o, "{ a: number; b: string; }");
AssertType([o], "{ a: number; b: string; }[]");
AssertType(o, "{ a: number; b: string; }");

let t3: { c: number; d: string };
AssertType(t3, "{ c: number; d: string; }");
AssertType(c, "number");
AssertType(d, "string");

let t4 = x.g<number, string>([o], [o]);
AssertType(t4, "{ c: number; d: string; }");
AssertType(x.g<number, string>([o], [o]), "{ c: number; d: string; }");
AssertType(x.g, "<T, U>(...{ a: T; b: U; }[][]) => { c: T; d: U; }");
AssertType([o], "{ a: number; b: string; }[]");
AssertType(o, "{ a: number; b: string; }");
AssertType([o], "{ a: number; b: string; }[]");
AssertType(o, "{ a: number; b: string; }");

let t4: { c: number; d: string };
AssertType(t4, "{ c: number; d: string; }");
AssertType(c, "number");
AssertType(d, "string");


