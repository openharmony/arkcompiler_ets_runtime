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

// === tests/cases/compiler/underscoreThisInDerivedClass01.ts ===
declare function AssertType(value:any, type:string):void;
// @target es5

// Original test intent:
// When arrow functions capture 'this', the lexical 'this' owner
// currently captures 'this' using a letiable named '_this'.
// That means that '_this' becomes a reserved identifier in certain places.
//
// Constructors have adopted the same identifier name ('_this')
// for capturing any potential return values from super calls,
// so we expect the same behavior.

class C {
    constructor() {
AssertType({}, "{}");
        return {};
    }
}

class D extends C {
    constructor() {
        let _this = "uh-oh?";
AssertType(_this, "string");
AssertType("uh-oh?", "string");

        console.log("ruh-roh...");
AssertType(console.log("ruh-roh..."), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("ruh-roh...", "string");

        super();
AssertType(super(), "void");
AssertType(super, "typeof C");

        console.log("d'oh!");
AssertType(console.log("d'oh!"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("d'oh!", "string");
    }
}

