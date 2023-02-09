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

// === tests/cases/conformance/esnext/logicalAssignment/logicalAssignment11.ts ===
declare function AssertType(value:any, type:string):void;
let x: string | undefined;
AssertType(x, "union");

let d: string | undefined;
AssertType(d, "union");

d ?? (d = x ?? "x")
AssertType(d ?? (d = x ?? "x"), "string");
AssertType(d, "union");
AssertType((d = x ?? "x"), "string");
AssertType(d = x ?? "x", "string");
AssertType(d, "union");
AssertType(x ?? "x", "string");
AssertType(x, "union");
AssertType("x", "string");

d.length;
AssertType(d.length, "number");

let e: string | undefined;
AssertType(e, "union");

e ??= x ?? "x"
AssertType(e ??= x ?? "x", "string");
AssertType(e, "union");
AssertType(x ?? "x", "string");
AssertType(x, "union");
AssertType("x", "string");

e.length
AssertType(e.length, "number");


