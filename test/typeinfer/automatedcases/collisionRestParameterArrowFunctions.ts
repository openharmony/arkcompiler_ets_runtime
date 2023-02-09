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

// === tests/cases/compiler/collisionRestParameterArrowFunctions.ts ===
declare function AssertType(value:any, type:string):void;
let f1 = (_i: number, ...restParameters) => { //_i is error
AssertType(f1, "(number, ...any[]) => void");
AssertType((_i: number, ...restParameters) => { //_i is error    let _i = 10; // no error}, "(number, ...any[]) => void");
AssertType(_i, "number");
AssertType(restParameters, "any[]");

    let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
}
let f1NoError = (_i: number) => { // no error
AssertType(f1NoError, "(number) => void");
AssertType((_i: number) => { // no error    let _i = 10; // no error}, "(number) => void");
AssertType(_i, "number");

    let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
}

let f2 = (...restParameters) => {
AssertType(f2, "(...any[]) => void");
AssertType((...restParameters) => {    let _i = 10; // No Error}, "(...any[]) => void");
AssertType(restParameters, "any[]");

    let _i = 10; // No Error
AssertType(_i, "number");
AssertType(10, "int");
}
let f2NoError = () => {
AssertType(f2NoError, "() => void");
AssertType(() => {    let _i = 10; // no error}, "() => void");

    let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
}

