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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormNotExpr.ts ===
declare function AssertType(value:any, type:string):void;
let str: string;
AssertType(str, "string");

let bool: boolean;
AssertType(bool, "boolean");

let num: number;
AssertType(num, "number");

let strOrNum: string | number;
AssertType(strOrNum, "union");

let strOrNumOrBool: string | number | boolean;
AssertType(strOrNumOrBool, "union");

let numOrBool: number | boolean;
AssertType(numOrBool, "union");

// A type guard of the form !expr
// - when true, narrows the type of x by expr when false, or
// - when false, narrows the type of x by expr when true.

// !typeguard1
if (!(typeof strOrNum === "string")) {
    num === strOrNum; // number
AssertType(num === strOrNum, "boolean");
AssertType(num, "number");
AssertType(strOrNum, "number");
}
else {
    str = strOrNum; // string
AssertType(str = strOrNum, "string");
AssertType(str, "string");
AssertType(strOrNum, "string");
}
// !(typeguard1 || typeguard2)
if (!(typeof strOrNumOrBool === "string" || typeof strOrNumOrBool === "number")) {
    bool = strOrNumOrBool; // boolean
AssertType(bool = strOrNumOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(strOrNumOrBool, "boolean");
}
else {
    strOrNum = strOrNumOrBool; // string | number
AssertType(strOrNum = strOrNumOrBool, "union");
AssertType(strOrNum, "union");
AssertType(strOrNumOrBool, "union");
}
// !(typeguard1) || !(typeguard2)
if (!(typeof strOrNumOrBool !== "string") || !(typeof strOrNumOrBool !== "number")) {
    strOrNum = strOrNumOrBool; // string | number
AssertType(strOrNum = strOrNumOrBool, "union");
AssertType(strOrNum, "union");
AssertType(strOrNumOrBool, "union");
}
else {
    bool = strOrNumOrBool; // boolean
AssertType(bool = strOrNumOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(strOrNumOrBool, "boolean");
}
// !(typeguard1 && typeguard2)
if (!(typeof strOrNumOrBool !== "string" && typeof strOrNumOrBool !== "number")) {
    strOrNum = strOrNumOrBool; // string | number
AssertType(strOrNum = strOrNumOrBool, "union");
AssertType(strOrNum, "union");
AssertType(strOrNumOrBool, "union");
}
else {
    bool = strOrNumOrBool; // boolean
AssertType(bool = strOrNumOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(strOrNumOrBool, "boolean");
}
// !(typeguard1) && !(typeguard2)
if (!(typeof strOrNumOrBool === "string") && !(typeof strOrNumOrBool === "number")) {
    bool = strOrNumOrBool; // boolean
AssertType(bool = strOrNumOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(strOrNumOrBool, "boolean");
}
else {
    strOrNum = strOrNumOrBool; // string | number
AssertType(strOrNum = strOrNumOrBool, "union");
AssertType(strOrNum, "union");
AssertType(strOrNumOrBool, "union");
}
// !(typeguard1) && simpleExpr
if (!(typeof strOrNumOrBool === "string") && numOrBool !== strOrNumOrBool) {
    numOrBool = strOrNumOrBool; // number | boolean
AssertType(numOrBool = strOrNumOrBool, "union");
AssertType(numOrBool, "union");
AssertType(strOrNumOrBool, "union");
}
else {
    let r1: string | number | boolean = strOrNumOrBool; // string | number | boolean
AssertType(r1, "union");
AssertType(strOrNumOrBool, "union");
}

