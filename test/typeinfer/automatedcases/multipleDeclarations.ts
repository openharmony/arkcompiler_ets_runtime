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
function C() {
    this.m = null;
AssertType(this.m = null, "null");
AssertType(this.m, "any");
AssertType(this, "this");
AssertType(null, "null");
}
C.prototype.m = function() {
AssertType(C.prototype.m = function() {    this.nothing();}, "() => void");
AssertType(C.prototype.m, "any");
AssertType(function() {    this.nothing();}, "() => void");

    this.nothing();
AssertType(this.nothing(), "error");
AssertType(this.nothing, "error");
AssertType(this, "this");
}
class X {
    constructor() {
        this.m = this.m.bind(this);
AssertType(this.m = this.m.bind(this), "any");
AssertType(this.m, "() => void");
AssertType(this, "this");
AssertType(this.m.bind(this), "any");
AssertType(this.m.bind, "(Function, any, ...any[]) => any");
AssertType(this.m, "() => void");
AssertType(this, "this");
AssertType(this, "this");

        this.mistake = 'frankly, complete nonsense';
AssertType(this.mistake = 'frankly, complete nonsense', "string");
AssertType(this.mistake, "() => void");
AssertType(this, "this");
AssertType('frankly, complete nonsense', "string");
    }
    m() {
    }
    mistake() {
    }
}
let x = new X();
AssertType(x, "X");
AssertType(new X(), "X");
AssertType(X, "typeof X");

X.prototype.mistake = false;
AssertType(X.prototype.mistake = false, "boolean");
AssertType(X.prototype.mistake, "() => void");
AssertType(false, "boolean");

x.m();
AssertType(x.m(), "void");
AssertType(x.m, "() => void");

x.mistake;
AssertType(x.mistake, "() => void");

class Y {
    mistake() {
    }
    m() {
    }
    constructor() {
        this.m = this.m.bind(this);
AssertType(this.m = this.m.bind(this), "any");
AssertType(this.m, "() => void");
AssertType(this, "this");
AssertType(this.m.bind(this), "any");
AssertType(this.m.bind, "(Function, any, ...any[]) => any");
AssertType(this.m, "() => void");
AssertType(this, "this");
AssertType(this, "this");

        this.mistake = 'even more nonsense';
AssertType(this.mistake = 'even more nonsense', "string");
AssertType(this.mistake, "() => void");
AssertType(this, "this");
AssertType('even more nonsense', "string");
    }
}
Y.prototype.mistake = true;
AssertType(Y.prototype.mistake = true, "boolean");
AssertType(Y.prototype.mistake, "() => void");
AssertType(true, "boolean");

let y = new Y();
AssertType(y, "Y");
AssertType(new Y(), "Y");
AssertType(Y, "typeof Y");

y.m();
AssertType(y.m(), "void");
AssertType(y.m, "() => void");

y.mistake();
AssertType(y.mistake(), "void");
AssertType(y.mistake, "() => void");


