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

// === tests/cases/conformance/es6/modules/m1.ts ===
declare function AssertType(value:any, type:string):void;
declare let console: any;
AssertType(console, "any");

export function _() {
    console.log("_");
AssertType(console.log("_"), "any");
AssertType(console.log, "any");
AssertType("_", "string");
}
export function __() {
    console.log("__");
AssertType(console.log("__"), "any");
AssertType(console.log, "any");
AssertType("__", "string");
}
export function ___() {
    console.log("___");
AssertType(console.log("___"), "any");
AssertType(console.log, "any");
AssertType("___", "string");
}
export function _hi() {
    console.log("_hi");
AssertType(console.log("_hi"), "any");
AssertType(console.log, "any");
AssertType("_hi", "string");
}
export function __proto() {
    console.log("__proto");
AssertType(console.log("__proto"), "any");
AssertType(console.log, "any");
AssertType("__proto", "string");
}
export function __esmodule() {
    console.log("__esmodule");
AssertType(console.log("__esmodule"), "any");
AssertType(console.log, "any");
AssertType("__esmodule", "string");
}
export function ___hello(){
    console.log("___hello");
AssertType(console.log("___hello"), "any");
AssertType(console.log, "any");
AssertType("___hello", "string");
}


