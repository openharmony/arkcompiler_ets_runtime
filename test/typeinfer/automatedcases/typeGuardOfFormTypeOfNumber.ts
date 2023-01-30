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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormTypeOfNumber.ts ===
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
if (typeof strOrNum === "number") {
    num = strOrNum; // number
AssertType(num = strOrNum, "number");
AssertType(num, "number");
AssertType(strOrNum, "number");
}
else {
    str === strOrNum; // string
AssertType(str === strOrNum, "boolean");
AssertType(str, "string");
AssertType(strOrNum, "string");
}
if (typeof numOrBool === "number") {
    num = numOrBool; // number
AssertType(num = numOrBool, "number");
AssertType(num, "number");
AssertType(numOrBool, "number");
}
else {
    let x: number | boolean = numOrBool; // number | boolean
AssertType(x, "union");
AssertType(numOrBool, "boolean");
}
if (typeof strOrNumOrBool === "number") {
    num = strOrNumOrBool; // number
AssertType(num = strOrNumOrBool, "number");
AssertType(num, "number");
AssertType(strOrNumOrBool, "number");
}
else {
    strOrBool = strOrNumOrBool; // string | boolean
AssertType(strOrBool = strOrNumOrBool, "union");
AssertType(strOrBool, "union");
AssertType(strOrNumOrBool, "union");
}
if (typeof numOrC === "number") {
    num = numOrC; // number
AssertType(num = numOrC, "number");
AssertType(num, "number");
AssertType(numOrC, "number");
}
else {
    c = numOrC; // C
AssertType(c = numOrC, "C");
AssertType(c, "C");
AssertType(numOrC, "C");
}

if (typeof strOrBool === "number") {
    let y1: {} = strOrBool; // {
AssertType(y1, "{}");

AssertType(strOrBool, "never");
}
}
else {
    let y2: string | boolean = strOrBool; // string | boolean
AssertType(y2, "union");
AssertType(strOrBool, "union");
}

// A type guard of the form typeof x !== s, where s is a string literal,
//  - when true, narrows the type of x by typeof x === s when false, or
//  - when false, narrows the type of x by typeof x === s when true.
if (typeof strOrNum !== "number") {
    str === strOrNum; // string
AssertType(str === strOrNum, "boolean");
AssertType(str, "string");
AssertType(strOrNum, "string");
}
else {
    num = strOrNum; // number
AssertType(num = strOrNum, "number");
AssertType(num, "number");
AssertType(strOrNum, "number");
}
if (typeof numOrBool !== "number") {
    let x: number | boolean = numOrBool; // number | boolean
AssertType(x, "union");
AssertType(numOrBool, "boolean");
}
else {
    num = numOrBool; // number
AssertType(num = numOrBool, "number");
AssertType(num, "number");
AssertType(numOrBool, "number");
}
if (typeof strOrNumOrBool !== "number") {
    strOrBool = strOrNumOrBool; // string | boolean
AssertType(strOrBool = strOrNumOrBool, "union");
AssertType(strOrBool, "union");
AssertType(strOrNumOrBool, "union");
}
else {
    num = strOrNumOrBool; // number
AssertType(num = strOrNumOrBool, "number");
AssertType(num, "number");
AssertType(strOrNumOrBool, "number");
}
if (typeof numOrC !== "number") {
    c = numOrC; // C
AssertType(c = numOrC, "C");
AssertType(c, "C");
AssertType(numOrC, "C");
}
else {
    num = numOrC; // number
AssertType(num = numOrC, "number");
AssertType(num, "number");
AssertType(numOrC, "number");
}

if (typeof strOrBool !== "number") {
    let y1: string | boolean = strOrBool; // string | boolean
AssertType(y1, "union");
AssertType(strOrBool, "union");
}
else {
    let y2: {} = strOrBool; // {
AssertType(y2, "{}");

AssertType(strOrBool, "never");
}
}


