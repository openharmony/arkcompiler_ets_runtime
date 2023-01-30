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

// === tests/cases/conformance/externalModules/foo_1.ts ===
declare function AssertType(value:any, type:string):void;
import * as foo from "./foo_0";
// None of the below should cause a runtime dependency on foo_0
import f = foo.M1;
let i: f.I2;
AssertType(i, "f.I2");
AssertType(f, "any");

let x: foo.C1 = <{m1: number}>{};
AssertType(x, "foo.C1");
AssertType(foo, "any");
AssertType(<{m1: number}>{}, "{ m1: number; }");
AssertType(m1, "number");
AssertType({}, "{}");

let y: typeof foo.C1.s1 = false;
AssertType(y, "boolean");
AssertType(foo.C1.s1, "boolean");
AssertType(foo.C1, "typeof foo.C1");
AssertType(foo, "typeof foo");
AssertType(C1, "typeof foo.C1");
AssertType(s1, "boolean");
AssertType(false, "boolean");

let z: foo.M1.I2;
AssertType(z, "f.I2");
AssertType(foo, "any");
AssertType(M1, "any");

let e: number = <foo.E1>0;
AssertType(e, "number");
AssertType(<foo.E1>0, "foo.E1");
AssertType(foo, "any");
AssertType(0, "int");


