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

// === tests/cases/compiler/es6ImportDefaultBindingFollowedWithNamedImport_1.ts ===
declare function AssertType(value:any, type:string):void;
import defaultBinding1, { } from "./es6ImportDefaultBindingFollowedWithNamedImport_0";
import defaultBinding2, { a } from "./es6ImportDefaultBindingFollowedWithNamedImport_0";
let x1: number = a;
AssertType(x1, "number");
AssertType(a, "number");

import defaultBinding3, { a as b } from "./es6ImportDefaultBindingFollowedWithNamedImport_0";
let x1: number = b;
AssertType(x1, "number");
AssertType(b, "number");

import defaultBinding4, { x, a as y } from "./es6ImportDefaultBindingFollowedWithNamedImport_0";
let x1: number = x;
AssertType(x1, "number");
AssertType(x, "number");

let x1: number = y;
AssertType(x1, "number");
AssertType(y, "number");

import defaultBinding5, { x as z,  } from "./es6ImportDefaultBindingFollowedWithNamedImport_0";
let x1: number = z;
AssertType(x1, "number");
AssertType(z, "number");

import defaultBinding6, { m,  } from "./es6ImportDefaultBindingFollowedWithNamedImport_0";
let x1: number = m;
AssertType(x1, "number");
AssertType(m, "number");


