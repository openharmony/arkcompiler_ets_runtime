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

// === tests/cases/conformance/classes/members/privateNames/privateNamesAssertion.ts ===
declare function AssertType(value:any, type:string):void;
class Foo {
    #p1: (v: any) => asserts v is string = (v) => {
        if (typeof v !== "string") {
AssertType(typeof v !== "string", "boolean");
AssertType(typeof v, "union");
AssertType(v, "any");
AssertType("string", "string");

            throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
        }
    }
    m1(v: unknown) {
        this.#p1(v);
AssertType(this.#p1(v), "void");
AssertType(this.#p1, "(any) => asserts v is string");
AssertType(this, "this");
AssertType(v, "unknown");

        v;
AssertType(v, "string");
    }
}

class Foo2 {
    #p1(v: any): asserts v is string {
        if (typeof v !== "string") {
AssertType(typeof v !== "string", "boolean");
AssertType(typeof v, "union");
AssertType(v, "any");
AssertType("string", "string");

            throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
        }
    }
    m1(v: unknown) {
        this.#p1(v);
AssertType(this.#p1(v), "void");
AssertType(this.#p1, "(any) => asserts v is string");
AssertType(this, "this");
AssertType(v, "unknown");

        v;
AssertType(v, "string");
    }
}


