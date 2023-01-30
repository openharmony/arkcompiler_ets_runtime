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

// === tests/cases/conformance/jsdoc/declarations/jsDeclarationsClassMethod.js ===
declare function AssertType(value:any, type:string):void;
function C1() {
    /**
     * A comment prop
     * @param {number} x
     * @param {number} y
     * @
returns {number}
     */
    this.prop = function (x, y) {
AssertType(this.prop, "any");
AssertType(this, "this");
AssertType(function (x, y) {        return x + y;    }, "(number, number) => number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(this.prop = function (x, y) {        return x + y;    }, "(number, number) => number");

AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
        return x + y;
    }
}

/**
 * A comment method
 * @param {number} x
 * @param {number} y
 * @
returns {number}
 */
C1.prototype.method = function (x, y) {
AssertType(C1.prototype.method, "any");
AssertType(function (x, y) {    return x + y;}, "(number, number) => number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(C1.prototype.method = function (x, y) {    return x + y;}, "(number, number) => number");

AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
    return x + y;
}

/**
 * A comment staticProp
 * @param {number} x
 * @param {number} y
 * @
returns {number}
 */
C1.staticProp = function (x, y) {
AssertType(C1.staticProp, "(number, number) => number");
AssertType(function (x, y) {    return x + y;}, "(number, number) => number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(C1.staticProp = function (x, y) {    return x + y;}, "(number, number) => number");

AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
    return x + y;
}

class C2 {
    /**
     * A comment method1
     * @param {number} x
     * @param {number} y
     * @
returns {number}
     */
    method1(x, y) {
AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
        return x + y;
    }
}

/**
 * A comment method2
 * @param {number} x
 * @param {number} y
 * @
returns {number}
 */
C2.prototype.method2 = function (x, y) {
AssertType(C2.prototype.method2, "(number, number) => number");
AssertType(function (x, y) {    return x + y;}, "(number, number) => number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(C2.prototype.method2 = function (x, y) {    return x + y;}, "(number, number) => number");

AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
    return x + y;
}

/**
 * A comment staticProp
 * @param {number} x
 * @param {number} y
 * @
returns {number}
 */
C2.staticProp = function (x, y) {
AssertType(C2.staticProp, "(number, number) => number");
AssertType(function (x, y) {    return x + y;}, "(number, number) => number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(C2.staticProp = function (x, y) {    return x + y;}, "(number, number) => number");

AssertType(x + y, "number");
AssertType(x, "number");
AssertType(y, "number");
    return x + y;
}


