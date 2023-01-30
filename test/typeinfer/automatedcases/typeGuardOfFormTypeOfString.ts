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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormTypeOfString.ts ===
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
if (typeof strOrNum === "string") {
    str = strOrNum; // string
AssertType(str = strOrNum, "string");
AssertType(str, "string");
AssertType(strOrNum, "string");
}
else {
    num === strOrNum; // number
AssertType(num === strOrNum, "boolean");
AssertType(num, "number");
AssertType(strOrNum, "number");
}
if (typeof strOrBool === "string") {
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
if (typeof strOrNumOrBool === "string") {
    str = strOrNumOrBool; // string
AssertType(str = strOrNumOrBool, "string");
AssertType(str, "string");
AssertType(strOrNumOrBool, "string");
}
else {
    numOrBool = strOrNumOrBool; // number | boolean
AssertType(numOrBool = strOrNumOrBool, "union");
AssertType(numOrBool, "union");
AssertType(strOrNumOrBool, "union");
}
if (typeof strOrC === "string") {
    str = strOrC; // string
AssertType(str = strOrC, "string");
AssertType(str, "string");
AssertType(strOrC, "string");
}
else {
    c = strOrC; // C
AssertType(c = strOrC, "C");
AssertType(c, "C");
AssertType(strOrC, "C");
}

if (typeof numOrBool === "string") {
    let x1: {} = numOrBool; // {
AssertType(x1, "{}");

AssertType(numOrBool, "never");
}
}
else {
    let x2: number | boolean = numOrBool; // number | boolean
AssertType(x2, "union");
AssertType(numOrBool, "union");
}

// A type guard of the form typeof x !== s, where s is a string literal,
//  - when true, narrows the type of x by typeof x === s when false, or
//  - when false, narrows the type of x by typeof x === s when true.
if (typeof strOrNum !== "string") {
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
if (typeof strOrBool !== "string") {
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
if (typeof strOrNumOrBool !== "string") {
    numOrBool = strOrNumOrBool; // number | boolean
AssertType(numOrBool = strOrNumOrBool, "union");
AssertType(numOrBool, "union");
AssertType(strOrNumOrBool, "union");
}
else {
    str = strOrNumOrBool; // string
AssertType(str = strOrNumOrBool, "string");
AssertType(str, "string");
AssertType(strOrNumOrBool, "string");
}
if (typeof strOrC !== "string") {
    c = strOrC; // C
AssertType(c = strOrC, "C");
AssertType(c, "C");
AssertType(strOrC, "C");
}
else {
    str = strOrC; // string
AssertType(str = strOrC, "string");
AssertType(str, "string");
AssertType(strOrC, "string");
}

if (typeof numOrBool !== "string") {
    let x1: number | boolean = numOrBool; // number | boolean
AssertType(x1, "union");
AssertType(numOrBool, "union");
}
else {
    let x2: {} = numOrBool; // {
AssertType(x2, "{}");

AssertType(numOrBool, "never");
}
}


