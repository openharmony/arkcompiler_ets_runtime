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

// === tests/cases/conformance/jsdoc/0.js ===
declare function AssertType(value:any, type:string):void;
// @ts-check
/**
 * @param {number=} n
 * @param {string} [s]
 */
function foo(n, s) {}

foo();
AssertType(foo(), "void");
AssertType(foo, "(?number, ?string) => void");

foo(1);
AssertType(foo(1), "void");
AssertType(foo, "(?number, ?string) => void");
AssertType(1, "int");

foo(1, "hi");
AssertType(foo(1, "hi"), "void");
AssertType(foo, "(?number, ?string) => void");
AssertType(1, "int");
AssertType("hi", "string");


