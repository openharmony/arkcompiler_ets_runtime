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

// === tests/cases/conformance/es6/modules/t3.ts ===
declare function AssertType(value:any, type:string):void;
import * as a from "./t1";
a.default;
AssertType(a.default, "string");

import b from "./t1";
b;
AssertType(b, "string");

import * as c from "./t1";
c.default;
AssertType(c.default, "string");

import { default as d } from "./t1";
d;
AssertType(d, "string");

import e1, * as e2 from "./t1";
e1;
AssertType(e1, "string");

e2.default;
AssertType(e2.default, "string");

import f1, { default as f2 } from "./t1";
f1;
AssertType(f1, "string");

f2;
AssertType(f2, "string");

export { a, b, c, d, e1, e2, f1, f2 };


