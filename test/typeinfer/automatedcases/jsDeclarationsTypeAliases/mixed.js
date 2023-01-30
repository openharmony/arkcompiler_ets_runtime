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

// === tests/cases/conformance/jsdoc/declarations/mixed.js ===
declare function AssertType(value:any, type:string):void;
/**
 * @typedef {{x: string} | number | LocalThing | ExportedThing} SomeType
 */
/**
 * @param {number} x
 * @
returns {SomeType}
 */
function doTheThing(x) {
AssertType({x: ""+x}, "{ x: string; }");
AssertType(x, "string");
AssertType(""+x, "string");
AssertType("", "string");
AssertType(x, "number");
    return {x: ""+x};
}
class ExportedThing {
    z = "ok"
}
module.exports = {
AssertType(module.exports = {    doTheThing,    ExportedThing,}, "typeof module.exports");
AssertType(module.exports, "typeof module.exports");
AssertType({    doTheThing,    ExportedThing,}, "{ doTheThing: (number) => SomeType; ExportedThing: typeof ExportedThing; }");

    doTheThing,
AssertType(doTheThing, "(number) => SomeType");

    ExportedThing,
AssertType(ExportedThing, "typeof ExportedThing");

};
class LocalThing {
    y = "ok"
}


