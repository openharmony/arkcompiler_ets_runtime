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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormTypeOfIsOrderIndependent.ts ===
declare function AssertType(value:any, type:string):void;
let strOrNum: string | number;
AssertType(strOrNum, "union");

let strOrBool: string | boolean;
AssertType(strOrBool, "union");

let strOrFunc: string | (() => void);
AssertType(strOrFunc, "union");

let numOrBool: number | boolean
AssertType(numOrBool, "union");

let str: string;
AssertType(str, "string");

let num: number;
AssertType(num, "number");

let bool: boolean;
AssertType(bool, "boolean");

let func: () => void;
AssertType(func, "() => void");

if ("string" === typeof strOrNum) {
    str = strOrNum;
AssertType(str = strOrNum, "string");
AssertType(str, "string");
AssertType(strOrNum, "string");
}
else {
    num = strOrNum;
AssertType(num = strOrNum, "number");
AssertType(num, "number");
AssertType(strOrNum, "number");
}
if ("function" === typeof strOrFunc) {
    func = strOrFunc;
AssertType(func = strOrFunc, "() => void");
AssertType(func, "() => void");
AssertType(strOrFunc, "() => void");
}
else {
    str = strOrFunc;
AssertType(str = strOrFunc, "string");
AssertType(str, "string");
AssertType(strOrFunc, "string");
}
if ("number" === typeof numOrBool) {
    num = numOrBool;
AssertType(num = numOrBool, "number");
AssertType(num, "number");
AssertType(numOrBool, "number");
}
else {
    bool = numOrBool;
AssertType(bool = numOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(numOrBool, "boolean");
}
if ("boolean" === typeof strOrBool) {
    bool = strOrBool;
AssertType(bool = strOrBool, "boolean");
AssertType(bool, "boolean");
AssertType(strOrBool, "boolean");
}
else {
    str = strOrBool;
AssertType(str = strOrBool, "string");
AssertType(str, "string");
AssertType(strOrBool, "string");
}


