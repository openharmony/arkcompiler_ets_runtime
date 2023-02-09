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

// === tests/cases/conformance/salsa/lateBoundClassMemberAssignmentJS.js ===
declare function AssertType(value:any, type:string):void;
const _sym = Symbol("_sym");
AssertType(_sym, "unique symbol");
AssertType(Symbol("_sym"), "unique symbol");
AssertType(Symbol, "SymbolConstructor");
AssertType("_sym", "string");

export class MyClass {
    constructor() {
        let self = this
AssertType(self, "this");
AssertType(this, "this");

        self[_sym] = "ok";
AssertType(self[_sym] = "ok", "string");
AssertType(self[_sym], "string");
AssertType(self, "this");
AssertType(_sym, "unique symbol");
AssertType("ok", "string");
    }

    method() {
        let self = this
AssertType(self, "this");
AssertType(this, "this");

        self[_sym] = "yep";
AssertType(self[_sym] = "yep", "string");
AssertType(self[_sym], "string");
AssertType(self, "this");
AssertType(_sym, "unique symbol");
AssertType("yep", "string");

        const x = self[_sym];
AssertType(x, "string");
AssertType(self[_sym], "string");
AssertType(self, "this");
AssertType(_sym, "unique symbol");
    }
}


