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

// === tests/cases/compiler/shouldNotPrintNullEscapesIntoOctalLiterals.ts ===
declare function AssertType(value:any, type:string):void;
"use strict";
AssertType("use strict", "string");

`\x001`;
AssertType(`\x001`, "string");

`\u00001`;
AssertType(`\u00001`, "string");

`\u{00000000}1`;
AssertType(`\u{00000000}1`, "string");

`\u{000000}1`;
AssertType(`\u{000000}1`, "string");

`\u{0}1`;
AssertType(`\u{0}1`, "string");


