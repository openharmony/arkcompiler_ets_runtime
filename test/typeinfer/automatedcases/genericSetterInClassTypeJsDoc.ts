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

// === tests/cases/conformance/classes/members/classTypes/genericSetterInClassTypeJsDoc.js ===
declare function AssertType(value:any, type:string):void;
/**
 * @template T
 */
 class Box {
    #value;

    /** @param {T} initialValue */
    constructor(initialValue) {
        this.#value = initialValue;
AssertType(this.#value = initialValue, "T");
AssertType(this.#value, "T");
AssertType(this, "this");
AssertType(initialValue, "T");
    }
    
    /** @type {T} */
    get value() {
AssertType(this.#value, "T");
AssertType(this, "this");
        return this.#value;
    }

    set value(value) {
        this.#value = value;
AssertType(this.#value = value, "T");
AssertType(this.#value, "T");
AssertType(this, "this");
AssertType(value, "T");
    }
}

new Box(3).value = 3;
AssertType(new Box(3).value = 3, "int");
AssertType(new Box(3).value, "number");
AssertType(3, "int");


