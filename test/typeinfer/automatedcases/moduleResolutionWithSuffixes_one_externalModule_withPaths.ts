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

// === /node_modules/some-library/lib/index.ios.js ===
declare function AssertType(value:any, type:string):void;
"use strict";
AssertType("use strict", "string");

exports.__esModule = true;
AssertType(exports.__esModule = true, "boolean");
AssertType(exports.__esModule, "boolean");
AssertType(true, "boolean");

function ios() {}
exports.ios = ios;
AssertType(exports.ios = ios, "() => void");
AssertType(exports.ios, "() => void");
AssertType(ios, "() => void");


// === /node_modules/some-library/lib/index.ios.d.ts ===
declare function AssertType(value:any, type:string):void;

export declare function ios(): void;

// === /node_modules/some-library/lib/index.js ===
declare function AssertType(value:any, type:string):void;
"use strict";
AssertType("use strict", "string");

exports.__esModule = true;
AssertType(exports.__esModule = true, "boolean");
AssertType(exports.__esModule, "boolean");
AssertType(true, "boolean");

function base() {}
exports.base = base;
AssertType(exports.base = base, "() => void");
AssertType(exports.base, "() => void");
AssertType(base, "() => void");


// === /node_modules/some-library/lib/index.d.ts ===
declare function AssertType(value:any, type:string):void;

export declare function base(): void;


// === /test.ts ===
declare function AssertType(value:any, type:string):void;

import { ios } from "some-library";
import { ios as ios2 } from "some-library/index";
import { ios as ios3 } from "some-library/index.js";


