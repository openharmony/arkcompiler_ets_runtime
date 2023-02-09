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

// === tests/cases/conformance/types/intersection/intersectionTypeInference2.ts ===
declare function AssertType(value:any, type:string):void;
declare function f<T>(x: { prop: T }): T;

declare const a: { prop: string } & { prop: number };
AssertType(a, "{ prop: string; } & { prop: number; }");
AssertType(prop, "string");
AssertType(prop, "number");

declare const b: { prop: string & number };
AssertType(b, "{ prop: string & number; }");
AssertType(prop, "never");

f(a);  // never
AssertType(f(a), "never");
AssertType(f, "<T>({ prop: T; }) => T");
AssertType(a, "{ prop: string; } & { prop: number; }");

f(b);  // never
AssertType(f(b), "never");
AssertType(f, "<T>({ prop: T; }) => T");
AssertType(b, "{ prop: never; }");

// Repro from #18354

declare function f2<T, Key extends keyof T>(obj: {[K in keyof T]: T[K]}, key: Key): T[Key];

declare const obj: { a: string } & { b: string };
AssertType(obj, "{ a: string; } & { b: string; }");
AssertType(a, "string");
AssertType(b, "string");

f2(obj, 'a');
AssertType(f2(obj, 'a'), "string");
AssertType(f2, "<T, Key extends keyof T>({ [K in keyof T]: T[K]; }, Key) => T[Key]");
AssertType(obj, "{ a: string; } & { b: string; }");
AssertType('a', "string");

f2(obj, 'b');
AssertType(f2(obj, 'b'), "string");
AssertType(f2, "<T, Key extends keyof T>({ [K in keyof T]: T[K]; }, Key) => T[Key]");
AssertType(obj, "{ a: string; } & { b: string; }");
AssertType('b', "string");


