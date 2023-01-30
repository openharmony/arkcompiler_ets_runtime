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

// === tests/cases/conformance/salsa/thisPropertyAssignmentCircular.js ===
declare function AssertType(value:any, type:string):void;
export class Foo {
    constructor() {
        this.foo = "Hello";
AssertType(this.foo = "Hello", "string");
AssertType(this.foo, "any");
AssertType(this, "this");
AssertType("Hello", "string");
    }
    slicey() {
        this.foo = this.foo.slice();
AssertType(this.foo = this.foo.slice(), "string");
AssertType(this.foo, "string");
AssertType(this, "this");
AssertType(this.foo.slice(), "string");
AssertType(this.foo.slice, "(?number, ?number) => string");
AssertType(this.foo, "string");
AssertType(this, "this");
    }
    m() {
        this.foo
AssertType(this.foo, "string");
AssertType(this, "this");
    }
}

/** @class */
function C() {
    this.x = 0;
AssertType(this.x = 0, "int");
AssertType(this.x, "any");
AssertType(this, "this");
AssertType(0, "int");

    this.x = function() { this.x.toString(); 
AssertType(this.x = function() { this.x.toString(); }, "() => void");

AssertType(this.x, "any");

AssertType(this, "this");

AssertType(function() { this.x.toString(); }, "() => void");

AssertType(this.x.toString(), "string");

AssertType(this.x.toString, "() => string");

AssertType(this.x, "() => void");

AssertType(this, "this");
}
}


