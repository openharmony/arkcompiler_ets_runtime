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

// === tests/cases/compiler/unusedLocalsAndParametersTypeAliases.ts ===
declare function AssertType(value:any, type:string):void;
// used in a declaration
type handler1 = () => void;
export interface I1 {
    getHandler: handler1;
}

// exported
export type handler2 = () => void;

// used in extends clause
type handler3 = () => void;
export interface I3<T extends handler3> {
    getHandler: T;
}

// used in another type alias declaration
type handler4 = () => void;
type handler5 = handler4 | (()=>number);
let x: handler5;
AssertType(x, "handler5");

x();
AssertType(x(), "union");
AssertType(x, "handler5");

// used as type argument
type handler6 = () => void;
let y: Array<handler6>;
AssertType(y, "handler6[]");

y[0]();
AssertType(y[0](), "void");
AssertType(y[0], "handler6");
AssertType(y, "handler6[]");
AssertType(0, "int");
    

