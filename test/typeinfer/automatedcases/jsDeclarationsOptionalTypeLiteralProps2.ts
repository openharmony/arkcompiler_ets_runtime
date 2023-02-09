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

// === tests/cases/conformance/jsdoc/declarations/foo.js ===
declare function AssertType(value:any, type:string):void;
/**
 * foo
 *
 * @public
 * @param {object} opts
 * @param {number} opts.a
 * @param {number} [opts.b]
 * @param {number} [opts.c]
 * @
returns {number}
 */
function foo({ a, b, c }) {
AssertType(a + (b ?? 0) + (c ?? 0), "number");
AssertType(a + (b ?? 0), "number");
AssertType(a, "number");
AssertType((b ?? 0), "number");
AssertType(b ?? 0, "number");
AssertType(b, "union");
AssertType(0, "int");
AssertType((c ?? 0), "number");
AssertType(c ?? 0, "number");
AssertType(c, "union");
AssertType(0, "int");
    return a + (b ?? 0) + (c ?? 0);
}


