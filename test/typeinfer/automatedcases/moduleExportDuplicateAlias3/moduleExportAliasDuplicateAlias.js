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

// === tests/cases/conformance/salsa/moduleExportAliasDuplicateAlias.js ===
declare function AssertType(value:any, type:string):void;
exports.apply = undefined;
AssertType(exports.apply = undefined, "undefined");
AssertType(exports.apply, "union");
AssertType(undefined, "undefined");

exports.apply = undefined;
AssertType(exports.apply = undefined, "undefined");
AssertType(exports.apply, "union");
AssertType(undefined, "undefined");

function a() { }
exports.apply = a;
AssertType(exports.apply = a, "() => void");
AssertType(exports.apply, "union");
AssertType(a, "() => void");

exports.apply()
AssertType(exports.apply(), "void");
AssertType(exports.apply, "() => void");

exports.apply = 'ok'
AssertType(exports.apply = 'ok', "string");
AssertType(exports.apply, "union");
AssertType('ok', "string");

let OK = exports.apply.toUpperCase()
AssertType(OK, "string");
AssertType(exports.apply.toUpperCase(), "string");
AssertType(exports.apply.toUpperCase, "() => string");

exports.apply = 1
AssertType(exports.apply = 1, "int");
AssertType(exports.apply, "union");
AssertType(1, "int");


