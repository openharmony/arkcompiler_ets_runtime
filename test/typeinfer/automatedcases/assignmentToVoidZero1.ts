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

// === tests/cases/conformance/salsa/assignmentToVoidZero1.js ===
declare function AssertType(value:any, type:string):void;
// #38552
exports.y = exports.x = void 0;
AssertType(exports.y = exports.x = void 0, "undefined");
AssertType(exports.y, "int");
AssertType(exports.x = void 0, "undefined");
AssertType(exports.x, "int");
AssertType(void 0, "undefined");
AssertType(0, "int");

exports.x = 1;
AssertType(exports.x = 1, "int");
AssertType(exports.x, "int");
AssertType(1, "int");

exports.y = 2;
AssertType(exports.y = 2, "int");
AssertType(exports.y, "int");
AssertType(2, "int");


