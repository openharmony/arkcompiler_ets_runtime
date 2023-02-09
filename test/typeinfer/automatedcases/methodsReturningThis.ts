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

// === tests/cases/conformance/salsa/input.js ===
declare function AssertType(value:any, type:string):void;
function Class()
{
}

// error: 'Class' doesn't have property 'notPresent'
Class.prototype.containsError = function () { 
AssertType(Class.prototype.containsError = function () { return this.notPresent; }, "() => any");
AssertType(Class.prototype.containsError, "any");
AssertType(function () { return this.notPresent; }, "() => any");
AssertType(this.notPresent, "error");
AssertType(this, "this");
return this.notPresent; };

// lots of methods that return this, which caused out-of-memory in #9527
Class.prototype.m1 = function (a, b, c, d, tx, ty) { 
AssertType(Class.prototype.m1 = function (a, b, c, d, tx, ty) { return this; }, "(any, any, any, any, any, any) => this");
AssertType(Class.prototype.m1, "any");
AssertType(function (a, b, c, d, tx, ty) { return this; }, "(any, any, any, any, any, any) => this");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");
AssertType(d, "any");
AssertType(tx, "any");
AssertType(ty, "any");
AssertType(this, "this");
return this; };

Class.prototype.m2 = function (x, y) { 
AssertType(Class.prototype.m2 = function (x, y) { return this; }, "(any, any) => this");
AssertType(Class.prototype.m2, "any");
AssertType(function (x, y) { return this; }, "(any, any) => this");
AssertType(x, "any");
AssertType(y, "any");
AssertType(this, "this");
return this; };

Class.prototype.m3 = function (x, y) { 
AssertType(Class.prototype.m3 = function (x, y) { return this; }, "(any, any) => this");
AssertType(Class.prototype.m3, "any");
AssertType(function (x, y) { return this; }, "(any, any) => this");
AssertType(x, "any");
AssertType(y, "any");
AssertType(this, "this");
return this; };

Class.prototype.m4 = function (angle) { 
AssertType(Class.prototype.m4 = function (angle) { return this; }, "(any) => this");
AssertType(Class.prototype.m4, "any");
AssertType(function (angle) { return this; }, "(any) => this");
AssertType(angle, "any");
AssertType(this, "this");
return this; };

Class.prototype.m5 = function (matrix) { 
AssertType(Class.prototype.m5 = function (matrix) { return this; }, "(any) => this");
AssertType(Class.prototype.m5, "any");
AssertType(function (matrix) { return this; }, "(any) => this");
AssertType(matrix, "any");
AssertType(this, "this");
return this; };

Class.prototype.m6 = function (x, y, pivotX, pivotY, scaleX, scaleY, rotation, skewX, skewY) { 
AssertType(Class.prototype.m6 = function (x, y, pivotX, pivotY, scaleX, scaleY, rotation, skewX, skewY) { return this; }, "(any, any, any, any, any, any, any, any, any) => this");
AssertType(Class.prototype.m6, "any");
AssertType(function (x, y, pivotX, pivotY, scaleX, scaleY, rotation, skewX, skewY) { return this; }, "(any, any, any, any, any, any, any, any, any) => this");
AssertType(x, "any");
AssertType(y, "any");
AssertType(pivotX, "any");
AssertType(pivotY, "any");
AssertType(scaleX, "any");
AssertType(scaleY, "any");
AssertType(rotation, "any");
AssertType(skewX, "any");
AssertType(skewY, "any");
AssertType(this, "this");
return this; };

Class.prototype.m7 = function(matrix) { 
AssertType(Class.prototype.m7 = function(matrix) { return this; }, "(any) => this");
AssertType(Class.prototype.m7, "any");
AssertType(function(matrix) { return this; }, "(any) => this");
AssertType(matrix, "any");
AssertType(this, "this");
return this; };

Class.prototype.m8 = function() { 
AssertType(Class.prototype.m8 = function() { return this; }, "() => this");
AssertType(Class.prototype.m8, "any");
AssertType(function() { return this; }, "() => this");
AssertType(this, "this");
return this; };

Class.prototype.m9 = function () { 
AssertType(Class.prototype.m9 = function () { return this; }, "() => this");
AssertType(Class.prototype.m9, "any");
AssertType(function () { return this; }, "() => this");
AssertType(this, "this");
return this; };



