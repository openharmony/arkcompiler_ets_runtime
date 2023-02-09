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

// === tests/cases/conformance/es6/classDeclaration/emitClassDeclarationWithPropertyAssignmentInES6.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    x: string = "Hello world";
}

class D {
    x: string = "Hello world";
    y: number;
    constructor() {
        this.y = 10;
AssertType(this.y = 10, "int");
AssertType(this.y, "number");
AssertType(this, "this");
AssertType(10, "int");
    }
}

class E extends D{
    z: boolean = true;
}

class F extends D{
    z: boolean = true;
    j: string;
    constructor() {
        super();
AssertType(super(), "void");
AssertType(super, "typeof D");

        this.j = "HI";
AssertType(this.j = "HI", "string");
AssertType(this.j, "string");
AssertType(this, "this");
AssertType("HI", "string");
    }
}

