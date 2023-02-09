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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormFunctionEquality.ts ===
declare function AssertType(value:any, type:string):void;
declare function isString1(a: number, b: Object): b is string;

declare function isString2(a: Object): a is string;

switch (isString1(0, "")) {
    case isString2(""):
    default:
}

let x = isString1(0, "") === isString2("");
AssertType(x, "boolean");
AssertType(isString1(0, "") === isString2(""), "boolean");
AssertType(isString1(0, ""), "boolean");
AssertType(isString1, "(number, Object) => b is string");
AssertType(0, "int");
AssertType("", "string");
AssertType(isString2(""), "boolean");
AssertType(isString2, "(Object) => a is string");
AssertType("", "string");

function isString3(a: number, b: number, c: Object): c is string {
AssertType(isString1(0, c), "boolean");
AssertType(isString1, "(number, Object) => b is string");
AssertType(0, "int");
AssertType(c, "Object");
    return isString1(0, c);
}


