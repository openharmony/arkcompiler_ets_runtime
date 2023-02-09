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
module.exports = class {
AssertType(module.exports = class {    /**     * @param {number} p     */    constructor(p) {        this.t = 12 + p;    }}, "typeof import("tests/cases/conformance/jsdoc/declarations/index")");
AssertType(module.exports, "typeof import("tests/cases/conformance/jsdoc/declarations/index")");
AssertType(class {    /**     * @param {number} p     */    constructor(p) {        this.t = 12 + p;    }}, "typeof import("tests/cases/conformance/jsdoc/declarations/index")");

    /**
     * @param {number} p
     */
    constructor(p) {
AssertType(p, "number");

        this.t = 12 + p;
AssertType(this.t = 12 + p, "number");
AssertType(this.t, "any");
AssertType(this, "this");
AssertType(12 + p, "number");
AssertType(12, "int");
AssertType(p, "number");
    }
}
module.exports.Sub = class {
AssertType(module.exports.Sub = class {    constructor() {        this.instance = new module.exports(10);    }}, "typeof Sub");
AssertType(module.exports.Sub, "typeof Sub");
AssertType(class {    constructor() {        this.instance = new module.exports(10);    }}, "typeof Sub");

    constructor() {
        this.instance = new module.exports(10);
AssertType(this.instance = new module.exports(10), "import("tests/cases/conformance/jsdoc/declarations/index")");
AssertType(this.instance, "any");
AssertType(this, "this");
AssertType(new module.exports(10), "import("tests/cases/conformance/jsdoc/declarations/index")");
AssertType(module.exports, "typeof import("tests/cases/conformance/jsdoc/declarations/index")");
AssertType(10, "int");
    }
}


