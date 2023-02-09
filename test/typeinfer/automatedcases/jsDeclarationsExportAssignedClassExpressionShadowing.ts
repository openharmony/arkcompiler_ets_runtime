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
class A {
    member = new Q();
}
class Q {
    x = 42;
}
module.exports = class Q {
AssertType(module.exports = class Q {    constructor() {        this.x = new A();    }}, "typeof import("tests/cases/conformance/jsdoc/declarations/index")");
AssertType(module.exports, "typeof import("tests/cases/conformance/jsdoc/declarations/index")");
AssertType(class Q {    constructor() {        this.x = new A();    }}, "typeof import("tests/cases/conformance/jsdoc/declarations/index")");
AssertType(Q, "typeof import("tests/cases/conformance/jsdoc/declarations/index")");

    constructor() {
        this.x = new A();
AssertType(this.x = new A(), "A");
AssertType(this.x, "any");
AssertType(this, "this");
AssertType(new A(), "A");
AssertType(A, "typeof A");
    }
}
module.exports.Another = Q;
AssertType(module.exports.Another = Q, "typeof Q");
AssertType(module.exports.Another, "typeof Q");
AssertType(Q, "typeof Q");


