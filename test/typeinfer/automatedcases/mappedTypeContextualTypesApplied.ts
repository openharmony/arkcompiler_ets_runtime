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

// === tests/cases/compiler/mappedTypeContextualTypesApplied.ts ===
declare function AssertType(value:any, type:string):void;
type TakeString = (s: string) => any;

// Various functions accepting an object whose properties are TakeString functions.
// Note these all use mapped types.
declare function mapped1<T extends {[P in string]: TakeString}>(obj: T): void;
declare function mapped2<T extends {[P in keyof T]: TakeString}>(obj: T): void;
declare function mapped3<T extends {[P in keyof any]: TakeString}>(obj: T): void;
declare function mapped4<T>(obj: T & {[P in keyof T]: TakeString}): void;
declare function mapped5<T, K extends keyof T>(obj: T & {[P in K]: TakeString}): void;
declare function mapped6<K extends string>(obj: {[P in K]: TakeString}): void;
declare function mapped7<K extends keyof any>(obj: {[P in K]: TakeString}): void;
declare function mapped8<K extends 'foo'>(obj: {[P in K]: TakeString}): void;
declare function mapped9<K extends 'foo'|'bar'>(obj: {[P in K]: TakeString}): void;

mapped1({foo: s => 42});
AssertType(mapped1({foo: s => 42}), "void");
AssertType(mapped1, "<T extends { [string]: TakeString; }>(T) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");

mapped2({foo: s => 42});
AssertType(mapped2({foo: s => 42}), "void");
AssertType(mapped2, "<T extends { [P in keyof T]: TakeString; }>(T) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");

mapped3({foo: s => 42});
AssertType(mapped3({foo: s => 42}), "void");
AssertType(mapped3, "<T extends { [string]: TakeString; }>(T) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");

mapped4({foo: s => 42});
AssertType(mapped4({foo: s => 42}), "void");
AssertType(mapped4, "<T>(T & { [P in keyof T]: TakeString; }) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");

mapped5({foo: s => 42});
AssertType(mapped5({foo: s => 42}), "void");
AssertType(mapped5, "<T, K extends keyof T>(T & { [P in K]: TakeString; }) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");

mapped6({foo: s => 42});
AssertType(mapped6({foo: s => 42}), "void");
AssertType(mapped6, "<K extends string>({ [P in K]: TakeString; }) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");

mapped7({foo: s => 42});
AssertType(mapped7({foo: s => 42}), "void");
AssertType(mapped7, "<K extends union>({ [P in K]: TakeString; }) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");

mapped8({foo: s => 42});
AssertType(mapped8({foo: s => 42}), "void");
AssertType(mapped8, "<K extends "foo">({ [P in K]: TakeString; }) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");

mapped9({foo: s => 42});
AssertType(mapped9({foo: s => 42}), "void");
AssertType(mapped9, "<K extends union>({ [P in K]: TakeString; }) => void");
AssertType({foo: s => 42}, "{ foo: (string) => number; }");
AssertType(foo, "(string) => number");
AssertType(s => 42, "(string) => number");
AssertType(s, "string");
AssertType(42, "int");


