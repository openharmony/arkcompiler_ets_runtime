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

// === tests/cases/conformance/classes/members/privateNames/privateNameFieldDestructuredBinding.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    #field = 1;
    otherObject = new A();
    testObject() {
AssertType({ x: 10, y: 6 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(10, "int");
AssertType(y, "number");
AssertType(6, "int");
        return { x: 10, y: 6 };
    }
    testArray() {
AssertType([10, 11], "number[]");
AssertType(10, "int");
AssertType(11, "int");
        return [10, 11];
    }
    constructor() {
        let y: number;
AssertType(y, "number");

        ({ x: this.#field, y } = this.testObject());
AssertType(({ x: this.#field, y } = this.testObject()), "{ x: number; y: number; }");
AssertType({ x: this.#field, y } = this.testObject(), "{ x: number; y: number; }");
AssertType({ x: this.#field, y }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(y, "number");
AssertType(this.testObject(), "{ x: number; y: number; }");
AssertType(this.testObject, "() => { x: number; y: number; }");
AssertType(this, "this");

        ([this.#field, y] = this.testArray());
AssertType(([this.#field, y] = this.testArray()), "number[]");
AssertType([this.#field, y] = this.testArray(), "number[]");
AssertType([this.#field, y], "[number, number]");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(y, "number");
AssertType(this.testArray(), "number[]");
AssertType(this.testArray, "() => number[]");
AssertType(this, "this");

        ({ a: this.#field, b: [this.#field] } = { a: 1, b: [2] });
AssertType(({ a: this.#field, b: [this.#field] } = { a: 1, b: [2] }), "{ a: number; b: [number]; }");
AssertType({ a: this.#field, b: [this.#field] } = { a: 1, b: [2] }, "{ a: number; b: [number]; }");
AssertType({ a: this.#field, b: [this.#field] }, "{ a: number; b: [number]; }");
AssertType(a, "number");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(b, "[number]");
AssertType([this.#field], "[number]");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType({ a: 1, b: [2] }, "{ a: number; b: [number]; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "[number]");
AssertType([2], "[number]");
AssertType(2, "int");

        [this.#field, [this.#field]] = [1, [2]];
AssertType([this.#field, [this.#field]] = [1, [2]], "[number, [number]]");
AssertType([this.#field, [this.#field]], "[number, [number]]");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType([this.#field], "[number]");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType([1, [2]], "[number, [number]]");
AssertType(1, "int");
AssertType([2], "[number]");
AssertType(2, "int");

        ({ a: this.#field = 1, b: [this.#field = 1] } = { b: [] });
AssertType(({ a: this.#field = 1, b: [this.#field = 1] } = { b: [] }), "{ b: []; a?: number; }");
AssertType({ a: this.#field = 1, b: [this.#field = 1] } = { b: [] }, "{ b: []; a?: number; }");
AssertType({ a: this.#field = 1, b: [this.#field = 1] }, "{ a?: number; b: [number]; }");
AssertType(a, "number");
AssertType(this.#field = 1, "int");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(1, "int");
AssertType(b, "[number]");
AssertType([this.#field = 1], "[number]");
AssertType(this.#field = 1, "int");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(1, "int");
AssertType({ b: [] }, "{ b: []; a?: number; }");
AssertType(b, "[]");
AssertType([], "[]");

        [this.#field = 2] = [];
AssertType([this.#field = 2] = [], "[]");
AssertType([this.#field = 2], "[number]");
AssertType(this.#field = 2, "int");
AssertType(this.#field, "number");
AssertType(this, "this");
AssertType(2, "int");
AssertType([], "[]");

        [this.otherObject.#field = 2] = [];
AssertType([this.otherObject.#field = 2] = [], "[]");
AssertType([this.otherObject.#field = 2], "[number]");
AssertType(this.otherObject.#field = 2, "int");
AssertType(this.otherObject.#field, "number");
AssertType(this.otherObject, "A");
AssertType(this, "this");
AssertType(2, "int");
AssertType([], "[]");
    }
    static test(_a: A) {
        [_a.#field] = [2];
AssertType([_a.#field] = [2], "[number]");
AssertType([_a.#field], "[number]");
AssertType(_a.#field, "number");
AssertType([2], "[number]");
AssertType(2, "int");
    }
}


