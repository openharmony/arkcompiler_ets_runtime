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

// === tests/cases/conformance/es6/destructuring/emptyAssignmentPatterns02_ES5iterable.ts ===
declare function AssertType(value:any, type:string):void;
let a: any;
AssertType(a, "any");

let x, y, z, a1, a2, a3;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a1, "any");
AssertType(a2, "any");
AssertType(a3, "any");

({} = { x, y, z } = a);
AssertType(({} = { x, y, z } = a), "any");
AssertType({} = { x, y, z } = a, "any");
AssertType({}, "{}");
AssertType({ x, y, z } = a, "any");
AssertType({ x, y, z }, "{ x: any; y: any; z: any; }");
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");

([] = [ a1, a2, a3] = a);
AssertType(([] = [ a1, a2, a3] = a), "any");
AssertType([] = [ a1, a2, a3] = a, "any");
AssertType([], "[]");
AssertType([ a1, a2, a3] = a, "any");
AssertType([ a1, a2, a3], "[any, any, any]");
AssertType(a1, "any");
AssertType(a2, "any");
AssertType(a3, "any");
AssertType(a, "any");


