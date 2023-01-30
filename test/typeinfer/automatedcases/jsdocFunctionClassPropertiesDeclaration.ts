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

// === /a.js ===
declare function AssertType(value:any, type:string):void;
/**
 * @param {number | undefined} x
 * @param {number | undefined} y
 */
export function Foo(x, y) {
    if (!(this instanceof Foo)) {
AssertType(!(this instanceof Foo), "boolean");
AssertType((this instanceof Foo), "boolean");
AssertType(this instanceof Foo, "boolean");
AssertType(this, "this");
AssertType(Foo, "typeof Foo");

AssertType(new Foo(x, y), "Foo");
AssertType(Foo, "typeof Foo");
AssertType(x, "union");
AssertType(y, "union");
        return new Foo(x, y);
    }
    this.x = x;
AssertType(this.x = x, "union");
AssertType(this.x, "any");
AssertType(this, "this");
AssertType(x, "union");

    this.y = y;
AssertType(this.y = y, "union");
AssertType(this.y, "any");
AssertType(this, "this");
AssertType(y, "union");
}


