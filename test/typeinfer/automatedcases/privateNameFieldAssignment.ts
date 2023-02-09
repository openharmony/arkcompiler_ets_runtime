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

// === tests/cases/conformance/classes/members/privateNames/privateNameFieldAssignment.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    #field = 0;
    constructor() {
        this.#field = 1;
AssertType(this.#field = 1, "int");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(1, "int");

        this.#field += 2;
AssertType(this.#field += 2, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(2, "int");

        this.#field -= 3;
AssertType(this.#field -= 3, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(3, "int");

        this.#field /= 4;
AssertType(this.#field /= 4, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(4, "int");

        this.#field *= 5;
AssertType(this.#field *= 5, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(5, "int");

        this.#field **= 6;
AssertType(this.#field **= 6, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(6, "int");

        this.#field %= 7;
AssertType(this.#field %= 7, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(7, "int");

        this.#field <<= 8;
AssertType(this.#field <<= 8, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(8, "int");

        this.#field >>= 9;
AssertType(this.#field >>= 9, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(9, "int");

        this.#field >>>= 10;
AssertType(this.#field >>>= 10, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(10, "int");

        this.#field &= 11;
AssertType(this.#field &= 11, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(11, "int");

        this.#field |= 12;
AssertType(this.#field |= 12, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(12, "int");

        this.#field ^= 13;
AssertType(this.#field ^= 13, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(13, "int");

        A.getInstance().#field = 1;
AssertType(A.getInstance().#field = 1, "int");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(1, "int");

        A.getInstance().#field += 2;
AssertType(A.getInstance().#field += 2, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(2, "int");

        A.getInstance().#field -= 3;
AssertType(A.getInstance().#field -= 3, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(3, "int");

        A.getInstance().#field /= 4;
AssertType(A.getInstance().#field /= 4, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(4, "int");

        A.getInstance().#field *= 5;
AssertType(A.getInstance().#field *= 5, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(5, "int");

        A.getInstance().#field **= 6;
AssertType(A.getInstance().#field **= 6, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(6, "int");

        A.getInstance().#field %= 7;
AssertType(A.getInstance().#field %= 7, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(7, "int");

        A.getInstance().#field <<= 8;
AssertType(A.getInstance().#field <<= 8, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(8, "int");

        A.getInstance().#field >>= 9;
AssertType(A.getInstance().#field >>= 9, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(9, "int");

        A.getInstance().#field >>>= 10;
AssertType(A.getInstance().#field >>>= 10, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(10, "int");

        A.getInstance().#field &= 11;
AssertType(A.getInstance().#field &= 11, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(11, "int");

        A.getInstance().#field |= 12;
AssertType(A.getInstance().#field |= 12, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(12, "int");

        A.getInstance().#field ^= 13;
AssertType(A.getInstance().#field ^= 13, "number");
AssertType(A.getInstance().#field, "number");
AssertType(A.getInstance(), "A");
AssertType(A.getInstance, "() => A");
AssertType(13, "int");
    }
    static getInstance() {
AssertType(new A(), "A");
AssertType(A, "typeof A");
        return new A();
    }
}


