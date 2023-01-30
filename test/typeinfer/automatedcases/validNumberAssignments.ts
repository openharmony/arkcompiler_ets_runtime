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

// === tests/cases/conformance/types/primitives/number/validNumberAssignments.ts ===
declare function AssertType(value:any, type:string):void;
let x = 1;
AssertType(x, "number");
AssertType(1, "int");

let a: any = x;
AssertType(a, "any");
AssertType(x, "number");

let b: Object = x;
AssertType(b, "Object");
AssertType(x, "number");

let c: number = x;
AssertType(c, "number");
AssertType(x, "number");

enum E { A };
let d: E = x;
AssertType(d, "E");
AssertType(x, "number");

let e = E.A;
AssertType(e, "E");
AssertType(E.A, "E");

e = x;
AssertType(e = x, "number");
AssertType(e, "E");
AssertType(x, "number");


