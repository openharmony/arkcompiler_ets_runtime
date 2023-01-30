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

// === tests/cases/compiler/contextualTypeOfIndexedAccessParameter.ts ===
declare function AssertType(value:any, type:string):void;
type Keys = "a" | "b";

type OptionsForKey = { a: { cb: (p: number) => number } } & { b: {} };

declare function f<K extends Keys>(key: K, options: OptionsForKey[K]): void;

f("a", {
AssertType(f("a", {    cb: p => p,}), "void");
AssertType(f, "<K extends Keys>(K, OptionsForKey[K]) => void");
AssertType("a", "string");
AssertType({    cb: p => p,}, "{ cb: (number) => number; }");

    cb: p => p,
AssertType(cb, "(number) => number");
AssertType(p => p, "(number) => number");
AssertType(p, "number");
AssertType(p, "number");

});

function g<
    K extends "a" | "b">(x: ({ a: string } & { b: string })[K], y: string) {
    x = y;
AssertType(x = y, "string");
AssertType(x, "({ a: string; } & { b: string; })[K]");
AssertType(y, "string");
}


