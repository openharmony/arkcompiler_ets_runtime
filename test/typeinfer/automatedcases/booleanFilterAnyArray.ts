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

// === tests/cases/compiler/booleanFilterAnyArray.ts ===
declare function AssertType(value:any, type:string):void;
interface Bullean { }
interface BulleanConstructor {
    new(v1?: any): Bullean;
    <T>(v2?: T): v2 is T;
}

interface Ari<T> {
    filter<S extends T>(cb1: (value: T) => value is S): T extends any ? Ari<any> : Ari<S>;
    filter(cb2: (value: T) => unknown): Ari<T>;
}
declare let Bullean: BulleanConstructor;
AssertType(Bullean, "BulleanConstructor");

declare let anys: Ari<any>;
AssertType(anys, "Ari<any>");

let xs: Ari<any>;
AssertType(xs, "Ari<any>");

let xs = anys.filter(Bullean)
AssertType(xs, "Ari<any>");
AssertType(anys.filter(Bullean), "Ari<any>");
AssertType(anys.filter, "{ <S extends any>((any) => value is S): Ari<any>; ((any) => unknown): Ari<any>; }");
AssertType(Bullean, "BulleanConstructor");

declare let realanys: any[];
AssertType(realanys, "any[]");

let ys: any[];
AssertType(ys, "any[]");

let ys = realanys.filter(Boolean)
AssertType(ys, "any[]");
AssertType(realanys.filter(Boolean), "any[]");
AssertType(realanys.filter, "{ <S extends any>((any, number, any[]) => value is S, ?any): S[]; ((any, number, any[]) => unknown, ?any): any[]; }");
AssertType(Boolean, "BooleanConstructor");

let foo = [{ name: 'x' }]
AssertType(foo, "{ name: string; }[]");
AssertType([{ name: 'x' }], "{ name: string; }[]");
AssertType({ name: 'x' }, "{ name: string; }");
AssertType(name, "string");
AssertType('x', "string");

let foor: Array<{name: string}>
AssertType(foor, "{ name: string; }[]");
AssertType(name, "string");

let foor = foo.filter(x => x.name)
AssertType(foor, "{ name: string; }[]");
AssertType(foo.filter(x => x.name), "{ name: string; }[]");
AssertType(foo.filter, "{ <S extends { name: string; }>(({ name: string; }, number, { name: string; }[]) => value is S, ?any): S[]; (({ name: string; }, number, { name: string; }[]) => unknown, ?any): { name: string; }[]; }");
AssertType(x => x.name, "({ name: string; }) => string");
AssertType(x, "{ name: string; }");
AssertType(x.name, "string");

let foos: Array<boolean>
AssertType(foos, "boolean[]");

let foos = [true, true, false, null].filter((thing): thing is boolean => thing !== null)
AssertType(foos, "boolean[]");
AssertType([true, true, false, null].filter((thing): thing is boolean => thing !== null), "boolean[]");
AssertType([true, true, false, null].filter, "{ <S extends boolean>((boolean, number, boolean[]) => value is S, ?any): S[]; ((boolean, number, boolean[]) => unknown, ?any): boolean[]; }");
AssertType((thing): thing is boolean => thing !== null, "(boolean) => thing is boolean");
AssertType(thing, "boolean");
AssertType(thing !== null, "boolean");
AssertType(thing, "boolean");
AssertType(null, "null");


