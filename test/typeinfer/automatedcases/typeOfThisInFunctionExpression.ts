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

// === tests/cases/conformance/expressions/functions/typeOfThisInFunctionExpression.ts ===
declare function AssertType(value:any, type:string):void;
// type of 'this' in FunctionExpression is Any

function fn() {
    let p = this;
AssertType(p, "any");
AssertType(this, "any");

    let p: any;
AssertType(p, "any");
}

let t = function () {
AssertType(t, "() => void");
AssertType(function () {    let p = this;    let p: any;}, "() => void");

    let p = this;
AssertType(p, "any");
AssertType(this, "any");

    let p: any;
AssertType(p, "any");
}

let t2 = function f() {
AssertType(t2, "() => void");
AssertType(function f() {    let x = this;    let x: any;}, "() => void");
AssertType(f, "() => void");

    let x = this;
AssertType(x, "any");
AssertType(this, "any");

    let x: any;
AssertType(x, "any");
}

class C {
    x = function () {
        let q: any;
AssertType(q, "any");

        let q = this;
AssertType(q, "any");
AssertType(this, "any");
    }
    y = function ff() {
        let q: any;
AssertType(q, "any");

        let q = this;
AssertType(q, "any");
AssertType(this, "any");
    }
}

module M {
    function fn() {
        let p = this;
AssertType(p, "any");
AssertType(this, "any");

        let p: any;
AssertType(p, "any");
    }

    let t = function () {
        let p = this;
AssertType(p, "any");
AssertType(this, "any");

        let p: any;
AssertType(p, "any");
    }

    let t2 = function f() {
        let x = this;
AssertType(x, "any");
AssertType(this, "any");

        let x: any;
AssertType(x, "any");
    }

}

