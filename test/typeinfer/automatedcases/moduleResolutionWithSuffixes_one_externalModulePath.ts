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

// === /node_modules/some-library/foo.ios.js ===
declare function AssertType(value:any, type:string):void;
"use strict";
AssertType("use strict", "string");

exports.__esModule = true;
AssertType(exports.__esModule = true, "boolean");
AssertType(exports.__esModule, "boolean");
AssertType(true, "boolean");

function iosfoo() {}
exports.iosfoo = iosfoo;
AssertType(exports.iosfoo = iosfoo, "() => void");
AssertType(exports.iosfoo, "() => void");
AssertType(iosfoo, "() => void");


// === /node_modules/some-library/foo.ios.d.ts ===
declare function AssertType(value:any, type:string):void;

export declare function iosfoo(): void;

// === /node_modules/some-library/foo.js ===
declare function AssertType(value:any, type:string):void;
"use strict";
AssertType("use strict", "string");

exports.__esModule = true;
AssertType(exports.__esModule = true, "boolean");
AssertType(exports.__esModule, "boolean");
AssertType(true, "boolean");

function basefoo() {}
exports.basefoo = basefoo;
AssertType(exports.basefoo = basefoo, "() => void");
AssertType(exports.basefoo, "() => void");
AssertType(basefoo, "() => void");


// === /node_modules/some-library/foo.d.ts ===
declare function AssertType(value:any, type:string):void;

export declare function basefoo(): void;


// === /index.ts ===
declare function AssertType(value:any, type:string):void;

import { iosfoo } from "some-library/foo";


