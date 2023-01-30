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

// === tests/cases/conformance/jsdoc/declarations/referencer.js ===
declare function AssertType(value:any, type:string):void;
import {Point} from "./source";

/**
 * @param {Point} p
 */
export function magnitude(p) {
AssertType(Math.sqrt(p.x ** 2 + p.y ** 2), "number");
AssertType(Math.sqrt, "(number) => number");
AssertType(p.x ** 2 + p.y ** 2, "number");
AssertType(p.x ** 2, "number");
AssertType(p.x, "number");
AssertType(2, "int");
AssertType(p.y ** 2, "number");
AssertType(p.y, "number");
AssertType(2, "int");
    return Math.sqrt(p.x ** 2 + p.y ** 2);
}


