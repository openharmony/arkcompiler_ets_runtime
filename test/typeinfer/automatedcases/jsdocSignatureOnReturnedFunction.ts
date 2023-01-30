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

// === tests/cases/conformance/jsdoc/jsdocSignatureOnReturnedFunction.js ===
declare function AssertType(value:any, type:string):void;
function f1() {
    /**
     * @param {number} a
     * @param {number} b
     * @
returns {number}
     */
AssertType((a, b) => {        return a + b;    }, "(number, number) => number");
AssertType(a, "number");
AssertType(b, "number");
    return (a, b) => {

AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "number");
        return a + b;
    }
}

function f2() {
    /**
     * @param {number} a
     * @param {number} b
     * @
returns {number}
     */
AssertType(function (a, b){        return a + b;    }, "(number, number) => number");
AssertType(a, "number");
AssertType(b, "number");
    return function (a, b){

AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "number");
        return a + b;
    }
}

function f3() {
    /** @type {(a: number, b: number) => number} */
AssertType((a, b) => {        return a + b;    }, "(number, number) => number");
AssertType(a, "number");
AssertType(b, "number");
    return (a, b) => {

AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "number");
        return a + b;
    }
}

function f4() {
    /** @type {(a: number, b: number) => number} */
AssertType(function (a, b){        return a + b;    }, "(number, number) => number");
AssertType(a, "number");
AssertType(b, "number");
    return function (a, b){

AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "number");
        return a + b;
    }
}


