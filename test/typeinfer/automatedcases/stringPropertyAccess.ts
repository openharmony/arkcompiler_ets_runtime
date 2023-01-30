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

// === tests/cases/conformance/types/primitives/string/stringPropertyAccess.ts ===
declare function AssertType(value:any, type:string):void;
let x = '';
AssertType(x, "string");
AssertType('', "string");

let a = x.charAt(0);
AssertType(a, "string");
AssertType(x.charAt(0), "string");
AssertType(x.charAt, "(number) => string");
AssertType(0, "int");

let b = x.hasOwnProperty('charAt');
AssertType(b, "boolean");
AssertType(x.hasOwnProperty('charAt'), "boolean");
AssertType(x.hasOwnProperty, "(PropertyKey) => boolean");
AssertType('charAt', "string");

let c = x['charAt'](0);
AssertType(c, "string");
AssertType(x['charAt'](0), "string");
AssertType(x['charAt'], "(number) => string");
AssertType(x, "string");
AssertType('charAt', "string");
AssertType(0, "int");

let e = x['hasOwnProperty']('toFixed');
AssertType(e, "boolean");
AssertType(x['hasOwnProperty']('toFixed'), "boolean");
AssertType(x['hasOwnProperty'], "(PropertyKey) => boolean");
AssertType(x, "string");
AssertType('hasOwnProperty', "string");
AssertType('toFixed', "string");


