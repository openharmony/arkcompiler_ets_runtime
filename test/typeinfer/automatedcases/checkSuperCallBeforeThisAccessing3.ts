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

// === tests/cases/compiler/checkSuperCallBeforeThisAccessing3.ts ===
declare function AssertType(value:any, type:string):void;
class Based { }
class Derived extends Based {
    public x: number;
    constructor() {
        class innver {
            public y: boolean;
            constructor() {
                this.y = true;
AssertType(this.y = true, "boolean");
AssertType(this.y, "boolean");
AssertType(this, "this");
AssertType(true, "boolean");
            }
        }
        super();
AssertType(super(), "void");
AssertType(super, "typeof Based");

        this.x = 10;
AssertType(this.x = 10, "int");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(10, "int");

        let that = this;
AssertType(that, "this");
AssertType(this, "this");
    }
}

