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

// === tests/cases/conformance/async/es5/asyncArrowFunction/asyncUnParenthesizedArrowFunction_es5.ts ===
declare function AssertType(value:any, type:string):void;
declare function someOtherFunction(i: any): Promise<void>;
const x = async i => await someOtherFunction(i)
AssertType(x, "(any) => Promise<void>");
AssertType(async i => await someOtherFunction(i), "(any) => Promise<void>");
AssertType(i, "any");
AssertType(await someOtherFunction(i), "void");
AssertType(someOtherFunction(i), "Promise<void>");
AssertType(someOtherFunction, "(any) => Promise<void>");
AssertType(i, "any");

const x1 = async (i) => await someOtherFunction(i);
AssertType(x1, "(any) => Promise<void>");
AssertType(async (i) => await someOtherFunction(i), "(any) => Promise<void>");
AssertType(i, "any");
AssertType(await someOtherFunction(i), "void");
AssertType(someOtherFunction(i), "Promise<void>");
AssertType(someOtherFunction, "(any) => Promise<void>");
AssertType(i, "any");


