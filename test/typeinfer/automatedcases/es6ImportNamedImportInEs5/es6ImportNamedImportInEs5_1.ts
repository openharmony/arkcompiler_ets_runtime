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

// === tests/cases/compiler/es6ImportNamedImportInEs5_1.ts ===
declare function AssertType(value:any, type:string):void;
import { } from "./es6ImportNamedImportInEs5_0";
import { a } from "./es6ImportNamedImportInEs5_0";
let xxxx = a;
AssertType(xxxx, "number");
AssertType(a, "number");

import { a as b } from "./es6ImportNamedImportInEs5_0";
let xxxx = b;
AssertType(xxxx, "number");
AssertType(b, "number");

import { x, a as y } from "./es6ImportNamedImportInEs5_0";
let xxxx = x;
AssertType(xxxx, "number");
AssertType(x, "number");

let xxxx = y;
AssertType(xxxx, "number");
AssertType(y, "number");

import { x as z,  } from "./es6ImportNamedImportInEs5_0";
let xxxx = z;
AssertType(xxxx, "number");
AssertType(z, "number");

import { m,  } from "./es6ImportNamedImportInEs5_0";
let xxxx = m;
AssertType(xxxx, "number");
AssertType(m, "number");

import { a1, x1 } from "./es6ImportNamedImportInEs5_0";
let xxxx = a1;
AssertType(xxxx, "number");
AssertType(a1, "number");

let xxxx = x1;
AssertType(xxxx, "number");
AssertType(x1, "number");

import { a1 as a11, x1 as x11 } from "./es6ImportNamedImportInEs5_0";
let xxxx = a11;
AssertType(xxxx, "number");
AssertType(a11, "number");

let xxxx = x11;
AssertType(xxxx, "number");
AssertType(x11, "number");

import { z1 } from "./es6ImportNamedImportInEs5_0";
let z111 = z1;
AssertType(z111, "number");
AssertType(z1, "number");

import { z2 as z3 } from "./es6ImportNamedImportInEs5_0";
let z2 = z3; // z2 shouldn't give redeclare error
AssertType(z2, "number");
AssertType(z3, "number");

// These are elided
import { aaaa } from "./es6ImportNamedImportInEs5_0";
// These are elided
import { aaaa as bbbb } from "./es6ImportNamedImportInEs5_0";


