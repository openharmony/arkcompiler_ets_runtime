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

// === tests/cases/conformance/salsa/inferringClassMembersFromAssignments8.ts ===
declare function AssertType(value:any, type:string):void;
// no inference in TS files, even for `this` aliases:

let app = function() {
AssertType(app, "() => void");
AssertType(function() {    let _this = this;    _this.swap = function() { }}, "() => void");

    let _this = this;
AssertType(_this, "any");
AssertType(this, "any");

    _this.swap = function() { 
AssertType(_this.swap = function() { }, "() => void");

AssertType(_this.swap, "any");

AssertType(function() { }, "() => void");
}
}
let a = new app()
AssertType(a, "any");
AssertType(new app(), "any");
AssertType(app, "() => void");

a
AssertType(a, "any");


