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

// === tests/cases/compiler/constructorWithSuperAndPrologue.es5.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/48761
"use strict";
AssertType("use strict", "string");

class A {
    public constructor() {
        console.log("A")
AssertType(console.log("A"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("A", "string");
    }
}

class B extends A  {
    constructor() {
        "ngInject";
AssertType("ngInject", "string");

        console.log("B")
AssertType(console.log("B"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("B", "string");

        super();
AssertType(super(), "void");
AssertType(super, "typeof A");
    }
}


