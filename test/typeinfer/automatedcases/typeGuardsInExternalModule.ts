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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInExternalModule.ts ===
declare function AssertType(value:any, type:string):void;
// Note that type guards affect types of letiables and parameters only and 
// have no effect on members of objects such as properties. 

// local letiable in external module
let num: number;
AssertType(num, "number");

let let1: string | number;
AssertType(let1, "union");

if (typeof let1 === "string") {
    num = let1.length; // string
AssertType(num = let1.length, "number");
AssertType(num, "number");
AssertType(let1.length, "number");
}
else {
    num = let1; // number
AssertType(num = let1, "number");
AssertType(num, "number");
AssertType(let1, "number");
}

// exported letiable in external module
let strOrNum: string | number;
AssertType(strOrNum, "union");

export let let2: string | number;
AssertType(let2, "union");

if (typeof let2 === "string") {
    // export makes the let property and not letiable
    strOrNum = let2; // string | number
AssertType(strOrNum = let2, "string");
AssertType(strOrNum, "union");
AssertType(let2, "string");
}
else {
    strOrNum = let2; // number | string
AssertType(strOrNum = let2, "number");
AssertType(strOrNum, "union");
AssertType(let2, "number");
}

