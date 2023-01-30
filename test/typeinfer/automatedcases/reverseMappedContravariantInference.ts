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

// === tests/cases/compiler/reverseMappedContravariantInference.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #21273

declare function conforms<T>(source: { [K in keyof T]: (val: T[K]) => boolean }): (value: T) => boolean;
conforms({ foo: (v: string) => false })({ foo: "hello" });
AssertType(conforms({ foo: (v: string) => false })({ foo: "hello" }), "boolean");
AssertType(conforms({ foo: (v: string) => false }), "({ foo: string; }) => boolean");
AssertType(conforms, "<T>({ [K in keyof T]: (T[K]) => boolean; }) => (T) => boolean");
AssertType({ foo: (v: string) => false }, "{ foo: (string) => false; }");
AssertType(foo, "(string) => false");
AssertType((v: string) => false, "(string) => false");
AssertType(v, "string");
AssertType(false, "boolean");
AssertType({ foo: "hello" }, "{ foo: string; }");
AssertType(foo, "string");
AssertType("hello", "string");


