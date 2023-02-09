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

// === tests/cases/conformance/jsdoc/in.js ===
declare function AssertType(value:any, type:string):void;
/**
 * @param {'literal'} p1
 * @param {"literal"} p2
 * @param {'literal' | 'other'} p3
 * @param {'literal' | number} p4
 * @param {12 | true | 'str'} p5
 */
function f(p1, p2, p3, p4, p5) {
AssertType(p1 + p2 + p3 + p4 + p5 + '.', "string");
AssertType(p1 + p2 + p3 + p4 + p5, "string");
AssertType(p1 + p2 + p3 + p4, "string");
AssertType(p1 + p2 + p3, "string");
AssertType(p1 + p2, "string");
AssertType(p1, "string");
AssertType(p2, "string");
AssertType(p3, "union");
AssertType(p4, "union");
AssertType(p5, "union");
AssertType('.', "string");
    return p1 + p2 + p3 + p4 + p5 + '.';
}


