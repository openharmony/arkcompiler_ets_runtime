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

// === tests/cases/conformance/jsdoc/declarations/index.js ===
declare function AssertType(value:any, type:string):void;
/** @type {?} */
export const a = null;
AssertType(a, "any");
AssertType(null, "null");

/** @type {*} */
export const b = null;
AssertType(b, "any");
AssertType(null, "null");

/** @type {string?} */
export const c = null;
AssertType(c, "string");
AssertType(null, "null");

/** @type {string=} */
export const d = null;
AssertType(d, "string");
AssertType(null, "null");

/** @type {string!} */
export const e = null;
AssertType(e, "string");
AssertType(null, "null");

/** @type {function(string, number): object} */
export const f = null;
AssertType(f, "(string, number) => any");
AssertType(null, "null");

/** @type {function(new: object, string, number)} */
export const g = null;
AssertType(g, "new (string, number) => any");
AssertType(null, "null");

/** @type {Object.<string, number>} */
export const h = null;
AssertType(h, "{ [string]: number; }");
AssertType(null, "null");


