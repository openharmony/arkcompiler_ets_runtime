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
class С1 {
    /** @type {string=} */
    p1 = undefined;

    /** @type {string | undefined} */
    p2 = undefined;

    /** @type {?string} */
    p3 = null;

    /** @type {string | null} */
    p4 = null;
}

class С2 {
    /** @type {string=} */
    get p1() {
AssertType(undefined, "undefined");
        return undefined;
    }

    /** @type {string | undefined} */
    get p2() {
AssertType(undefined, "undefined");
        return undefined;
    }

    /** @type {?string} */
    get p3() {
AssertType(null, "null");
        return null;
    }

    /** @type {string | null} */
    get p4() {
AssertType(null, "null");
        return null;
    }
}


class С3 {
    /** @type {string=} */
    get p1() {
AssertType(undefined, "undefined");
        return undefined;
    }

    /** @param {string=} value */
    set p1(value) {
        this.p1 = value;
AssertType(this.p1 = value, "union");
AssertType(this.p1, "union");
AssertType(this, "this");
AssertType(value, "union");
    }

    /** @type {string | undefined} */
    get p2() {
AssertType(undefined, "undefined");
        return undefined;
    }

    /** @param {string | undefined} value */
    set p2(value) {
        this.p2 = value;
AssertType(this.p2 = value, "union");
AssertType(this.p2, "union");
AssertType(this, "this");
AssertType(value, "union");
    }

    /** @type {?string} */
    get p3() {
AssertType(null, "null");
        return null;
    }

    /** @param {?string} value */
    set p3(value) {
        this.p3 = value;
AssertType(this.p3 = value, "union");
AssertType(this.p3, "union");
AssertType(this, "this");
AssertType(value, "union");
    }

    /** @type {string | null} */
    get p4() {
AssertType(null, "null");
        return null;
    }

    /** @param {string | null} value */
    set p4(value) {
        this.p4 = value;
AssertType(this.p4 = value, "union");
AssertType(this.p4, "union");
AssertType(this, "this");
AssertType(value, "union");
    }
}


class С4 {
    /** @param {string=} value */
    set p1(value) {
        this.p1 = value;
AssertType(this.p1 = value, "union");
AssertType(this.p1, "union");
AssertType(this, "this");
AssertType(value, "union");
    }

    /** @param {string | undefined} value */
    set p2(value) {
        this.p2 = value;
AssertType(this.p2 = value, "union");
AssertType(this.p2, "union");
AssertType(this, "this");
AssertType(value, "union");
    }

    /** @param {?string} value */
    set p3(value) {
        this.p3 = value;
AssertType(this.p3 = value, "union");
AssertType(this.p3, "union");
AssertType(this, "this");
AssertType(value, "union");
    }

    /** @param {string | null} value */
    set p4(value) {
        this.p4 = value;
AssertType(this.p4 = value, "union");
AssertType(this.p4, "union");
AssertType(this, "this");
AssertType(value, "union");
    }
}


