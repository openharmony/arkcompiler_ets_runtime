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

// === tests/cases/compiler/assignmentNestedInLiterals.ts ===
declare function AssertType(value:any, type:string):void;
let target, x, y;
AssertType(target, "any");
AssertType(x, "any");
AssertType(y, "any");

target = [x = 1, y = x];
AssertType(target = [x = 1, y = x], "number[]");
AssertType(target, "any");
AssertType([x = 1, y = x], "number[]");
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");
AssertType(y = x, "number");
AssertType(y, "any");
AssertType(x, "number");

let aegis, a, b;
AssertType(aegis, "any");
AssertType(a, "any");
AssertType(b, "any");

aegis = { x: a = 1, y: b = a };
AssertType(aegis = { x: a = 1, y: b = a }, "{ x: number; y: number; }");
AssertType(aegis, "any");
AssertType({ x: a = 1, y: b = a }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(a = 1, "int");
AssertType(a, "any");
AssertType(1, "int");
AssertType(y, "number");
AssertType(b = a, "number");
AssertType(b, "any");
AssertType(a, "number");

let kowloona, c, d;
AssertType(kowloona, "any");
AssertType(c, "any");
AssertType(d, "any");

for (kowloona of [c = 1, d = c]) {
}


