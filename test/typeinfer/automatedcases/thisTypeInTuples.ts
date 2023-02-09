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

// === tests/cases/conformance/types/thisType/thisTypeInTuples.ts ===
declare function AssertType(value:any, type:string):void;
interface Array<T> {
    slice(): this;
}

let t: [number, string] = [42, "hello"];
AssertType(t, "[number, string]");
AssertType([42, "hello"], "[number, string]");
AssertType(42, "int");
AssertType("hello", "string");

let a = t.slice();
AssertType(a, "[number, string]");
AssertType(t.slice(), "[number, string]");
AssertType(t.slice, "{ (?number, ?number): (union)[]; (): [number, string]; }");

let b = t.slice(1);
AssertType(b, "(union)[]");
AssertType(t.slice(1), "(union)[]");
AssertType(t.slice, "{ (?number, ?number): (union)[]; (): [number, string]; }");
AssertType(1, "int");

let c = t.slice(0, 1);
AssertType(c, "(union)[]");
AssertType(t.slice(0, 1), "(union)[]");
AssertType(t.slice, "{ (?number, ?number): (union)[]; (): [number, string]; }");
AssertType(0, "int");
AssertType(1, "int");


