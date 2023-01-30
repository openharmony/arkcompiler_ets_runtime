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

// === tests/cases/compiler/client.ts ===
declare function AssertType(value:any, type:string):void;
import { } from "./server";
import { a } from "./server";
export let xxxx = new a();
AssertType(xxxx, "a");
AssertType(new a(), "a");
AssertType(a, "typeof a");

import { a11 as b } from "./server";
export let xxxx1 = new b();
AssertType(xxxx1, "b");
AssertType(new b(), "b");
AssertType(b, "typeof b");

import { x, a12 as y } from "./server";
export let xxxx2 = new x();
AssertType(xxxx2, "x");
AssertType(new x(), "x");
AssertType(x, "typeof x");

export let xxxx3 = new y();
AssertType(xxxx3, "y");
AssertType(new y(), "y");
AssertType(y, "typeof y");

import { x11 as z,  } from "./server";
export let xxxx4 = new z();
AssertType(xxxx4, "z");
AssertType(new z(), "z");
AssertType(z, "typeof z");

import { m,  } from "./server";
export let xxxx5 = new m();
AssertType(xxxx5, "m");
AssertType(new m(), "m");
AssertType(m, "typeof m");

import { a1, x1 } from "./server";
export let xxxx6 = new a1();
AssertType(xxxx6, "a1");
AssertType(new a1(), "a1");
AssertType(a1, "typeof a1");

export let xxxx7 = new x1();
AssertType(xxxx7, "x1");
AssertType(new x1(), "x1");
AssertType(x1, "typeof x1");

import { a111 as a11, x111 as x11 } from "./server";
export let xxxx8 = new a11();
AssertType(xxxx8, "a11");
AssertType(new a11(), "a11");
AssertType(a11, "typeof a11");

export let xxxx9 = new x11();
AssertType(xxxx9, "x11");
AssertType(new x11(), "x11");
AssertType(x11, "typeof x11");

import { z1 } from "./server";
export let z111 = new z1();
AssertType(z111, "z1");
AssertType(new z1(), "z1");
AssertType(z1, "typeof z1");

import { z2 as z3 } from "./server";
export let z2 = new z3(); // z2 shouldn't give redeclare error
AssertType(z2, "z3");
AssertType(new z3(), "z3");
AssertType(z3, "typeof z3");

// not referenced
import { aaaa } from "./server";
import { aaaa1 as bbbb } from "./server";


