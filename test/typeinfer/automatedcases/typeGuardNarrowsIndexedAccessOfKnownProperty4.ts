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

// === tests/cases/compiler/typeGuardNarrowsIndexedAccessOfKnownProperty4.ts ===
declare function AssertType(value:any, type:string):void;
class Foo {
    x: number | undefined;

    constructor() {
        this.x = 5;
AssertType(this.x = 5, "int");
AssertType(this.x, "union");
AssertType(this, "this");
AssertType(5, "int");

        this.x;    // number
AssertType(this.x, "number");
AssertType(this, "this");

        this['x']; // number
AssertType(this['x'], "number");
AssertType(this, "this");
AssertType('x', "string");

        const key = 'x';
AssertType(key, "string");
AssertType('x', "string");

        this[key]; // number
AssertType(this[key], "number");
AssertType(this, "this");
AssertType(key, "string");
    }
}


