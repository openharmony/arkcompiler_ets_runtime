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

// === tests/cases/compiler/asyncFunctionContextuallyTypedReturns.ts ===
declare function AssertType(value:any, type:string):void;
declare function f(cb: (v: boolean) => [0] | PromiseLike<[0]>): void;
f(v => v ? [0] : Promise.reject());
AssertType(f(v => v ? [0] : Promise.reject()), "void");
AssertType(f, "((boolean) => union) => void");
AssertType(v => v ? [0] : Promise.reject(), "(boolean) => union");
AssertType(v, "boolean");
AssertType(v ? [0] : Promise.reject(), "union");
AssertType(v, "boolean");
AssertType([0], "[0]");
AssertType(0, "int");
AssertType(Promise.reject(), "Promise<[0]>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");

f(async v => v ? [0] : Promise.reject());
AssertType(f(async v => v ? [0] : Promise.reject()), "void");
AssertType(f, "((boolean) => union) => void");
AssertType(async v => v ? [0] : Promise.reject(), "(boolean) => Promise<[0]>");
AssertType(v, "boolean");
AssertType(v ? [0] : Promise.reject(), "union");
AssertType(v, "boolean");
AssertType([0], "[0]");
AssertType(0, "int");
AssertType(Promise.reject(), "Promise<[0]>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");

declare function g(cb: (v: boolean) => "contextuallyTypable" | PromiseLike<"contextuallyTypable">): void;
g(v => v ? "contextuallyTypable" : Promise.reject());
AssertType(g(v => v ? "contextuallyTypable" : Promise.reject()), "void");
AssertType(g, "((boolean) => union) => void");
AssertType(v => v ? "contextuallyTypable" : Promise.reject(), "(boolean) => union");
AssertType(v, "boolean");
AssertType(v ? "contextuallyTypable" : Promise.reject(), "union");
AssertType(v, "boolean");
AssertType("contextuallyTypable", "string");
AssertType(Promise.reject(), "Promise<"contextuallyTypable">");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");

g(async v => v ? "contextuallyTypable" : Promise.reject());
AssertType(g(async v => v ? "contextuallyTypable" : Promise.reject()), "void");
AssertType(g, "((boolean) => union) => void");
AssertType(async v => v ? "contextuallyTypable" : Promise.reject(), "(boolean) => Promise<"contextuallyTypable">");
AssertType(v, "boolean");
AssertType(v ? "contextuallyTypable" : Promise.reject(), "union");
AssertType(v, "boolean");
AssertType("contextuallyTypable", "string");
AssertType(Promise.reject(), "Promise<"contextuallyTypable">");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");

type MyCallback = (thing: string) => void;
declare function h(cb: (v: boolean) => MyCallback | PromiseLike<MyCallback>): void;
h(v => v ? (abc) => { } : Promise.reject());
AssertType(h(v => v ? (abc) => { } : Promise.reject()), "void");
AssertType(h, "((boolean) => union) => void");
AssertType(v => v ? (abc) => { } : Promise.reject(), "(boolean) => union");
AssertType(v, "boolean");
AssertType(v ? (abc) => { } : Promise.reject(), "union");
AssertType(v, "boolean");
AssertType((abc) => { }, "(string) => void");
AssertType(abc, "string");
AssertType(Promise.reject(), "Promise<MyCallback>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");

h(async v => v ? (def) => { } : Promise.reject());
AssertType(h(async v => v ? (def) => { } : Promise.reject()), "void");
AssertType(h, "((boolean) => union) => void");
AssertType(async v => v ? (def) => { } : Promise.reject(), "(boolean) => Promise<union>");
AssertType(v, "boolean");
AssertType(v ? (def) => { } : Promise.reject(), "union");
AssertType(v, "boolean");
AssertType((def) => { }, "(string) => void");
AssertType(def, "string");
AssertType(Promise.reject(), "Promise<MyCallback>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");


