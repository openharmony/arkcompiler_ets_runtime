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

// === tests/cases/conformance/salsa/propertyAssignmentUseParentType1.ts ===
declare function AssertType(value:any, type:string):void;
interface N {
    (): boolean
    num: 123;
}
export const interfaced: N = () => true;
AssertType(interfaced, "N");
AssertType(() => true, "{ (): true; num: 123; }");
AssertType(true, "boolean");

interfaced.num = 123;
AssertType(interfaced.num = 123, "int");
AssertType(interfaced.num, "int");
AssertType(123, "int");

export const inlined: { (): boolean; nun: 456 } = () => true;
AssertType(inlined, "{ (): boolean; nun: 456; }");
AssertType(nun, "int");
AssertType(() => true, "{ (): true; nun: 456; }");
AssertType(true, "boolean");

inlined.nun = 456;
AssertType(inlined.nun = 456, "int");
AssertType(inlined.nun, "int");
AssertType(456, "int");

export const ignoreJsdoc = () => true;
AssertType(ignoreJsdoc, "{ (): boolean; extra: number; }");
AssertType(() => true, "{ (): boolean; extra: number; }");
AssertType(true, "boolean");

/** @type {string} make sure to ignore jsdoc! */
ignoreJsdoc.extra = 111
AssertType(ignoreJsdoc.extra = 111, "int");
AssertType(ignoreJsdoc.extra, "number");
AssertType(111, "int");


