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

// === tests/cases/conformance/expressions/binaryOperators/instanceofOperator/instanceofOperatorWithLHSIsObject.ts ===
declare function AssertType(value:any, type:string):void;
class C { }

let x1: any;
AssertType(x1, "any");

let x2: Function;
AssertType(x2, "Function");

let a: {};
AssertType(a, "{}");

let b: Object;
AssertType(b, "Object");

let c: C;
AssertType(c, "C");

let d: string | C;
AssertType(d, "union");

let r1 = a instanceof x1;
AssertType(r1, "boolean");
AssertType(a instanceof x1, "boolean");
AssertType(a, "{}");
AssertType(x1, "any");

let r2 = b instanceof x2;
AssertType(r2, "boolean");
AssertType(b instanceof x2, "boolean");
AssertType(b, "Object");
AssertType(x2, "Function");

let r3 = c instanceof x1;
AssertType(r3, "boolean");
AssertType(c instanceof x1, "boolean");
AssertType(c, "C");
AssertType(x1, "any");

let r4 = d instanceof x1;
AssertType(r4, "boolean");
AssertType(d instanceof x1, "boolean");
AssertType(d, "union");
AssertType(x1, "any");


