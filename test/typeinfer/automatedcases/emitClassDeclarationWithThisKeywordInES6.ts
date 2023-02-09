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

// === tests/cases/conformance/es6/classDeclaration/emitClassDeclarationWithThisKeywordInES6.ts ===
declare function AssertType(value:any, type:string):void;
class B {
    x = 10;
    constructor() {
        this.x = 10;
AssertType(this.x = 10, "int");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(10, "int");
    }
    static log(a: number) { }
    foo() {
        B.log(this.x);
AssertType(B.log(this.x), "void");
AssertType(B.log, "(number) => void");
AssertType(this.x, "number");
AssertType(this, "this");
    }

    get X() {
AssertType(this.x, "number");
AssertType(this, "this");
        return this.x;
    }

    set bX(y: number) {
        this.x = y;
AssertType(this.x = y, "number");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(y, "number");
    }
}

