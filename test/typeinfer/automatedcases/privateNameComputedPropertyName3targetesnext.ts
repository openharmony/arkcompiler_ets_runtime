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

// === tests/cases/conformance/classes/members/privateNames/privateNameComputedPropertyName3.ts ===
declare function AssertType(value:any, type:string):void;
class Foo {
    #name;

    constructor(name) {
        this.#name = name;
AssertType(this.#name = name, "any");
AssertType(this.#name, "any");
AssertType(this, "this");
AssertType(name, "any");
    }

    getValue(x) {
        const obj = this;
AssertType(obj, "this");
AssertType(this, "this");

        class Bar {
            #y = 100;

            [obj.#name]() {
AssertType(x + this.#y, "any");
AssertType(x, "any");
AssertType(this.#y, "number");
AssertType(this, "this");
                return x + this.#y;
            }
        }

AssertType(new Bar()[obj.#name](), "error");
AssertType(new Bar()[obj.#name], "error");
AssertType(new Bar(), "Bar");
AssertType(Bar, "typeof Bar");
AssertType(obj.#name, "any");
        return new Bar()[obj.#name]();
    }
}

console.log(new Foo("NAME").getValue(100));
AssertType(console.log(new Foo("NAME").getValue(100)), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(new Foo("NAME").getValue(100), "error");
AssertType(new Foo("NAME").getValue, "(any) => any");
AssertType(100, "int");


