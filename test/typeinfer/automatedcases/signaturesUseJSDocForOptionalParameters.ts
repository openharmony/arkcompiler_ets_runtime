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

// === tests/cases/compiler/jsDocOptionality.js ===
declare function AssertType(value:any, type:string):void;
function MyClass() {
  this.prop = null;
AssertType(this.prop = null, "null");
AssertType(this.prop, "any");
AssertType(this, "this");
AssertType(null, "null");
}
/**
 * @param  {string} required
 * @param  {string} [notRequired]
 * @
returns {MyClass}
 */
MyClass.prototype.optionalParam = function(required, notRequired) {
AssertType(MyClass.prototype.optionalParam, "any");
AssertType(function(required, notRequired) {    return this;}, "(string, ?string) => MyClass");
AssertType(required, "string");
AssertType(notRequired, "string");
AssertType(MyClass.prototype.optionalParam = function(required, notRequired) {    return this;}, "(string, ?string) => MyClass");

AssertType(this, "this");
    return this;

};
let pInst = new MyClass();
AssertType(pInst, "MyClass");
AssertType(new MyClass(), "MyClass");
AssertType(MyClass, "typeof MyClass");

let c1 = pInst.optionalParam('hello')
AssertType(c1, "MyClass");
AssertType(pInst.optionalParam('hello'), "MyClass");
AssertType(pInst.optionalParam, "(string, ?string) => MyClass");
AssertType('hello', "string");

let c2 = pInst.optionalParam('hello', null)
AssertType(c2, "MyClass");
AssertType(pInst.optionalParam('hello', null), "MyClass");
AssertType(pInst.optionalParam, "(string, ?string) => MyClass");
AssertType('hello', "string");
AssertType(null, "null");


