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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormExpr1AndExpr2.ts ===
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

class C { private p; }
let c: C;
AssertType(c, "C");

let cOrBool: C| boolean;
AssertType(cOrBool, "union");

let strOrNumOrBoolOrC: string | number | boolean | C;
AssertType(strOrNumOrBoolOrC, "union");

// A type guard of the form expr1 && expr2
//  - when true, narrows the type of x by expr1 when true and then by expr2 when true, or
//  - when false, narrows the type of x to T1 | T2, where T1 is the type of x narrowed by expr1 when 
//    false, and T2 is the type of x narrowed by expr1 when true and then by expr2 when false.

// (typeguard1 && typeguard2)
if (typeof strOrNumOrBool !== "string" && typeof strOrNumOrBool !== "number") {
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
// (typeguard1 && typeguard2 && typeguard3)
if (typeof strOrNumOrBoolOrC !== "string" && typeof strOrNumOrBoolOrC !== "number" && typeof strOrNumOrBoolOrC !== "boolean") {
    c = strOrNumOrBoolOrC; // C
AssertType(c = strOrNumOrBoolOrC, "C");
AssertType(c, "C");
AssertType(strOrNumOrBoolOrC, "C");
}
else {
    strOrNumOrBool = strOrNumOrBoolOrC; // string | number | boolean
AssertType(strOrNumOrBool = strOrNumOrBoolOrC, "union");
AssertType(strOrNumOrBool, "union");
AssertType(strOrNumOrBoolOrC, "union");
}
// (typeguard1 && typeguard2 && typeguard11(onAnotherType))
if (typeof strOrNumOrBoolOrC !== "string" && typeof strOrNumOrBoolOrC !== "number" && typeof strOrNumOrBool === "boolean") {
    cOrBool = strOrNumOrBoolOrC; // C | boolean
AssertType(cOrBool = strOrNumOrBoolOrC, "union");
AssertType(cOrBool, "union");
AssertType(strOrNumOrBoolOrC, "union");

    bool = strOrNumOrBool; // boolean
AssertType(bool = strOrNumOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(strOrNumOrBool, "boolean");
}
else {
    let r1: string | number | boolean | C = strOrNumOrBoolOrC; // string | number | boolean | C
AssertType(r1, "union");
AssertType(strOrNumOrBoolOrC, "union");

    let r2: string | number | boolean = strOrNumOrBool;
AssertType(r2, "union");
AssertType(strOrNumOrBool, "union");
}
// (typeguard1) && simpleExpr
if (typeof strOrNumOrBool !== "string" && numOrBool !== strOrNumOrBool) {
    numOrBool = strOrNumOrBool; // number | boolean
AssertType(numOrBool = strOrNumOrBool, "union");
AssertType(numOrBool, "union");
AssertType(strOrNumOrBool, "union");
}
else {
    let r3: string | number | boolean = strOrNumOrBool; // string | number | boolean
AssertType(r3, "union");
AssertType(strOrNumOrBool, "union");
}

