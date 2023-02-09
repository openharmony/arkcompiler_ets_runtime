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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormTypeOfBoolean.ts ===
declare function AssertType(value:any, type:string):void;
class C { private p: string };

let str: string;
AssertType(str, "string");

let bool: boolean;
AssertType(bool, "boolean");

let num: number;
AssertType(num, "number");

let strOrNum: string | number;
AssertType(strOrNum, "union");

let strOrBool: string | boolean;
AssertType(strOrBool, "union");

let numOrBool: number | boolean
AssertType(numOrBool, "union");

let strOrNumOrBool: string | number | boolean;
AssertType(strOrNumOrBool, "union");

let strOrC: string | C;
AssertType(strOrC, "union");

let numOrC: number | C;
AssertType(numOrC, "union");

let boolOrC: boolean | C;
AssertType(boolOrC, "union");

let c: C;
AssertType(c, "C");

//	A type guard of the form typeof x === s, 
//  where s is a string literal with the value 'string', 'number', or 'boolean',
//  - when true, narrows the type of x to the given primitive type, or
//  - when false, removes the primitive type from the type of x.
if (typeof strOrBool === "boolean") {
    bool = strOrBool; // boolean
AssertType(bool = strOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(strOrBool, "boolean");
}
else {
    str = strOrBool; // string
AssertType(str = strOrBool, "string");
AssertType(str, "string");
AssertType(strOrBool, "string");
}
if (typeof numOrBool === "boolean") {
    bool = numOrBool; // boolean
AssertType(bool = numOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(numOrBool, "boolean");
}
else {
    num = numOrBool; // number
AssertType(num = numOrBool, "number");
AssertType(num, "number");
AssertType(numOrBool, "number");
}
if (typeof strOrNumOrBool === "boolean") {
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
if (typeof boolOrC === "boolean") {
    bool = boolOrC; // boolean
AssertType(bool = boolOrC, "boolean");
AssertType(bool, "boolean");
AssertType(boolOrC, "boolean");
}
else {
    c = boolOrC; // C
AssertType(c = boolOrC, "C");
AssertType(c, "C");
AssertType(boolOrC, "C");
}

if (typeof strOrNum === "boolean") {
    let z1: {} = strOrNum; // {
AssertType(z1, "{}");

AssertType(strOrNum, "never");
}
}
else {
    let z2: string | number = strOrNum; // string | number
AssertType(z2, "union");
AssertType(strOrNum, "union");
}


// A type guard of the form typeof x !== s, where s is a string literal,
//  - when true, narrows the type of x by typeof x === s when false, or
//  - when false, narrows the type of x by typeof x === s when true.
if (typeof strOrBool !== "boolean") {
    str = strOrBool; // string
AssertType(str = strOrBool, "string");
AssertType(str, "string");
AssertType(strOrBool, "string");
}
else {
    bool = strOrBool; // boolean
AssertType(bool = strOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(strOrBool, "boolean");
}
if (typeof numOrBool !== "boolean") {
    num = numOrBool; // number
AssertType(num = numOrBool, "number");
AssertType(num, "number");
AssertType(numOrBool, "number");
}
else {
    bool = numOrBool; // boolean
AssertType(bool = numOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(numOrBool, "boolean");
}
if (typeof strOrNumOrBool !== "boolean") {
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
if (typeof boolOrC !== "boolean") {
    c = boolOrC; // C
AssertType(c = boolOrC, "C");
AssertType(c, "C");
AssertType(boolOrC, "C");
}
else {
    bool = boolOrC; // boolean
AssertType(bool = boolOrC, "boolean");
AssertType(bool, "boolean");
AssertType(boolOrC, "boolean");
}

if (typeof strOrNum !== "boolean") {
    let z1: string | number = strOrNum; // string | number
AssertType(z1, "union");
AssertType(strOrNum, "union");
}
else {
    let z2: {} = strOrNum; // {
AssertType(z2, "{}");

AssertType(strOrNum, "never");
}
}


