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

// === tests/cases/compiler/inferentialTypingWithFunctionTypeNested.ts ===
declare function AssertType(value:any, type:string):void;
declare function map<T, U>(x: T, f: () => { x: (s: T) => U }): U;
declare function identity<V>(y: V): V;

let s = map("", () => { 
AssertType(s, "string");
AssertType(map("", () => { return { x: identity }; }), "string");
AssertType(map, "<T, U>(T, () => { x: (T) => U; }) => U");
AssertType("", "string");
AssertType(() => { return { x: identity }; }, "() => { x: (string) => string; }");
AssertType({ x: identity }, "{ x: <V>(V) => V; }");
AssertType(x, "<V>(V) => V");
AssertType(identity, "<V>(V) => V");
return { x: identity }; });


