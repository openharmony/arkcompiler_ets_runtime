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

// === tests/cases/compiler/genericIsNeverEmptyObject.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #29067

function test<T extends { a: string }>(obj: T) {
    let { a, ...rest } = obj;
AssertType(a, "string");
AssertType(rest, "Omit<T, "a">");
AssertType(obj, "T");

AssertType({ ...rest, b: a }, "Omit<T, "a"> & { b: string; }");
AssertType(rest, "Omit<T, "a">");
AssertType(b, "string");
AssertType(a, "string");
    return { ...rest, b: a };
}

let o1 = { a: 'hello', x: 42 };
AssertType(o1, "{ a: string; x: number; }");
AssertType({ a: 'hello', x: 42 }, "{ a: string; x: number; }");
AssertType(a, "string");
AssertType('hello', "string");
AssertType(x, "number");
AssertType(42, "int");

let o2: { b: string, x: number } = test(o1);
AssertType(o2, "{ b: string; x: number; }");
AssertType(b, "string");
AssertType(x, "number");
AssertType(test(o1), "Omit<{ a: string; x: number; }, "a"> & { b: string; }");
AssertType(test, "<T extends { a: string; }>(T) => Omit<T, "a"> & { b: string; }");
AssertType(o1, "{ a: string; x: number; }");


