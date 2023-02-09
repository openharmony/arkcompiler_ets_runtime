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

// === tests/cases/conformance/types/primitives/number/numberPropertyAccess.ts ===
declare function AssertType(value:any, type:string):void;
let x = 1;
AssertType(x, "number");
AssertType(1, "int");

let a = x.toExponential();
AssertType(a, "string");
AssertType(x.toExponential(), "string");
AssertType(x.toExponential, "(?number) => string");

let b = x.hasOwnProperty('toFixed');
AssertType(b, "boolean");
AssertType(x.hasOwnProperty('toFixed'), "boolean");
AssertType(x.hasOwnProperty, "(PropertyKey) => boolean");
AssertType('toFixed', "string");

let c = x['toExponential']();
AssertType(c, "string");
AssertType(x['toExponential'](), "string");
AssertType(x['toExponential'], "(?number) => string");
AssertType(x, "number");
AssertType('toExponential', "string");

let d = x['hasOwnProperty']('toFixed');
AssertType(d, "boolean");
AssertType(x['hasOwnProperty']('toFixed'), "boolean");
AssertType(x['hasOwnProperty'], "(PropertyKey) => boolean");
AssertType(x, "number");
AssertType('hasOwnProperty', "string");
AssertType('toFixed', "string");


