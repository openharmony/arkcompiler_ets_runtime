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

// === tests/cases/conformance/jsdoc/jsdocReadonlyDeclarations.js ===
declare function AssertType(value:any, type:string):void;
class C {
    /** @readonly */
    x = 6
    /** @readonly */
    constructor(n) {
        this.x = n
AssertType(this.x = n, "any");
AssertType(this.x, "int");
AssertType(this, "this");
AssertType(n, "any");

        /**
         * @readonly
         * @type {number}
         */
        this.y = n
AssertType(this.y = n, "any");
AssertType(this.y, "number");
AssertType(this, "this");
AssertType(n, "any");
    }
}
new C().x
AssertType(new C().x, "int");

function F() {
    /** @readonly */
    this.z = 1
AssertType(this.z = 1, "int");
AssertType(this.z, "any");
AssertType(this, "this");
AssertType(1, "int");
}

// https://github.com/microsoft/TypeScript/issues/38401
class D {
    constructor(/** @readonly */ x) {}
}

