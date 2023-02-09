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

// === tests/cases/conformance/jsdoc/declarations/file2.js ===
declare function AssertType(value:any, type:string):void;
const {myTypes} = require('./file.js');
AssertType(myTypes, "{ [string]: any; }");
AssertType(require('./file.js'), "typeof import("tests/cases/conformance/jsdoc/declarations/file")");
AssertType(require, "any");
AssertType('./file.js', "string");

/**
 * @namespace testFnTypes
 * @global
 * @type {Object<string,*>}
 */
const testFnTypes = {
AssertType(testFnTypes, "{ [string]: any; }");
AssertType({    // SOME PROPS HERE}, "{}");

    // SOME PROPS HERE
};

/** @typedef {boolean|myTypes.typeC} testFnTypes.input */

/**
 * @function testFn
 * @description A test function.
 * @param {testFnTypes.input} input - Input.
 * @
returns {number|null} Result.
 */
function testFn(input) {
    if (typeof input === 'number') {
AssertType(typeof input === 'number', "boolean");
AssertType(typeof input, "union");
AssertType(input, "union");
AssertType('number', "string");

AssertType(2 * input, "number");
AssertType(2, "int");
AssertType(input, "never");
        return 2 * input;

    } else {
AssertType(null, "null");
        return null;
    }
}

module.exports = {testFn, testFnTypes};
AssertType(module.exports = {testFn, testFnTypes}, "typeof module.exports");
AssertType(module.exports, "typeof module.exports");
AssertType({testFn, testFnTypes}, "{ testFn: (testFnTypes.input) => number; testFnTypes: { [string]: any; }; }");
AssertType(testFn, "(testFnTypes.input) => number");
AssertType(testFnTypes, "{ [string]: any; }");


