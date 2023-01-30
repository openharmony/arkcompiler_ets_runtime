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

// === tests/cases/compiler/typeArgInferenceWithNull.ts ===
declare function AssertType(value:any, type:string):void;
// All legal

function fn4<T extends string>(n: T) { }
fn4(null);
AssertType(fn4(null), "void");
AssertType(fn4, "<T extends string>(T) => void");
AssertType(null, "null");

function fn5<T extends { x: string }>(n: T) { }
fn5({ x: null });
AssertType(fn5({ x: null }), "void");
AssertType(fn5, "<T extends { x: string; }>(T) => void");
AssertType({ x: null }, "{ x: null; }");
AssertType(x, "null");
AssertType(null, "null");

function fn6<T extends { x: string }>(n: T, fun: (x: T) => void, n2: T) { }
fn6({ x: null }, y => { }, { x: "" }); // y has type { x: any }, but ideally would have type { x: string 
AssertType(fn6({ x: null }, y => { }, { x: "" }), "void");

AssertType(fn6, "<T extends { x: string; }>(T, (T) => void, T) => void");

AssertType({ x: null }, "{ x: null; }");

AssertType(x, "null");

AssertType(null, "null");

AssertType(y => { }, "({ x: string; }) => void");

AssertType(y, "{ x: string; }");

AssertType({ x: "" }, "{ x: string; }");

AssertType(x, "string");

AssertType("", "string");
}


