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

// === tests/cases/conformance/classes/members/privateNames/privateNameStaticFieldAssignment.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    static #field = 0;
    constructor() {
        A.#field = 1;
AssertType(A.#field = 1, "int");
AssertType(A.#field, "number");
AssertType(1, "int");

        A.#field += 2;
AssertType(A.#field += 2, "number");
AssertType(A.#field, "number");
AssertType(2, "int");

        A.#field -= 3;
AssertType(A.#field -= 3, "number");
AssertType(A.#field, "number");
AssertType(3, "int");

        A.#field /= 4;
AssertType(A.#field /= 4, "number");
AssertType(A.#field, "number");
AssertType(4, "int");

        A.#field *= 5;
AssertType(A.#field *= 5, "number");
AssertType(A.#field, "number");
AssertType(5, "int");

        A.#field **= 6;
AssertType(A.#field **= 6, "number");
AssertType(A.#field, "number");
AssertType(6, "int");

        A.#field %= 7;
AssertType(A.#field %= 7, "number");
AssertType(A.#field, "number");
AssertType(7, "int");

        A.#field <<= 8;
AssertType(A.#field <<= 8, "number");
AssertType(A.#field, "number");
AssertType(8, "int");

        A.#field >>= 9;
AssertType(A.#field >>= 9, "number");
AssertType(A.#field, "number");
AssertType(9, "int");

        A.#field >>>= 10;
AssertType(A.#field >>>= 10, "number");
AssertType(A.#field, "number");
AssertType(10, "int");

        A.#field &= 11;
AssertType(A.#field &= 11, "number");
AssertType(A.#field, "number");
AssertType(11, "int");

        A.#field |= 12;
AssertType(A.#field |= 12, "number");
AssertType(A.#field, "number");
AssertType(12, "int");

        A.#field ^= 13;
AssertType(A.#field ^= 13, "number");
AssertType(A.#field, "number");
AssertType(13, "int");

        A.getClass().#field = 1;
AssertType(A.getClass().#field = 1, "int");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(1, "int");

        A.getClass().#field += 2;
AssertType(A.getClass().#field += 2, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(2, "int");

        A.getClass().#field -= 3;
AssertType(A.getClass().#field -= 3, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(3, "int");

        A.getClass().#field /= 4;
AssertType(A.getClass().#field /= 4, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(4, "int");

        A.getClass().#field *= 5;
AssertType(A.getClass().#field *= 5, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(5, "int");

        A.getClass().#field **= 6;
AssertType(A.getClass().#field **= 6, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(6, "int");

        A.getClass().#field %= 7;
AssertType(A.getClass().#field %= 7, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(7, "int");

        A.getClass().#field <<= 8;
AssertType(A.getClass().#field <<= 8, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(8, "int");

        A.getClass().#field >>= 9;
AssertType(A.getClass().#field >>= 9, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(9, "int");

        A.getClass().#field >>>= 10;
AssertType(A.getClass().#field >>>= 10, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(10, "int");

        A.getClass().#field &= 11;
AssertType(A.getClass().#field &= 11, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(11, "int");

        A.getClass().#field |= 12;
AssertType(A.getClass().#field |= 12, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(12, "int");

        A.getClass().#field ^= 13;
AssertType(A.getClass().#field ^= 13, "number");
AssertType(A.getClass().#field, "number");
AssertType(A.getClass(), "typeof A");
AssertType(A.getClass, "() => typeof A");
AssertType(13, "int");
    }
    static getClass() {
AssertType(A, "typeof A");
        return A;
    }
}


