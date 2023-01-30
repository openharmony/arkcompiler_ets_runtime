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

// === tests/cases/compiler/typeArgumentsInFunctionExpressions.ts ===
declare function AssertType(value:any, type:string):void;
let obj = function f<T>(a: T) { // should not error
AssertType(obj, "<T>(T) => T");
AssertType(f, "<T>(T) => T");
AssertType(a, "T");
AssertType(function f<T>(a: T) { // should not error    let x: T;    return a;}, "<T>(T) => T");

    let x: T;
AssertType(x, "T");

AssertType(a, "T");
    return a;

};
 
let obj2 = function f<T>(a: T): T { // should not error
AssertType(obj2, "<T>(T) => T");
AssertType(f, "<T>(T) => T");
AssertType(a, "T");
AssertType(function f<T>(a: T): T { // should not error    let x: T;    return a;}, "<T>(T) => T");

    let x: T;
AssertType(x, "T");

AssertType(a, "T");
    return a;

};



