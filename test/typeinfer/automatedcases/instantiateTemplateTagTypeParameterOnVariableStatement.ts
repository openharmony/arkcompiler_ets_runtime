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

// === tests/cases/conformance/jsdoc/instantiateTemplateTagTypeParameterOnVariableStatement.js ===
declare function AssertType(value:any, type:string):void;
/**
 * @template T
 * @param {T} a
 * @
returns {(b: T) => T}
 */
const seq = a => b => b;
AssertType(seq, "<T>(T) => (T) => T");
AssertType(a => b => b, "<T>(T) => (T) => T");
AssertType(a, "T");
AssertType(b => b, "(T) => T");
AssertType(b, "T");
AssertType(b, "T");

const text1 = "hello";
AssertType(text1, "string");
AssertType("hello", "string");

const text2 = "world";
AssertType(text2, "string");
AssertType("world", "string");

/** @type {string} */
let text3 = seq(text1)(text2);
AssertType(text3, "string");
AssertType(seq(text1)(text2), "string");
AssertType(seq(text1), "(string) => string");
AssertType(seq, "<T>(T) => (T) => T");
AssertType(text1, "string");
AssertType(text2, "string");


