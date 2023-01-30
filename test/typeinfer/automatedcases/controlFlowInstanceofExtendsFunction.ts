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

// === tests/cases/conformance/controlFlow/controlFlowInstanceofExtendsFunction.ts ===
declare function AssertType(value:any, type:string):void;
declare global {
    interface Function {
        now(): string;
    }
}

Function.prototype.now = function () {
AssertType(Function.prototype.now, "() => string");
AssertType(function () {    return "now"}, "() => string");
AssertType(Function.prototype.now = function () {    return "now"}, "() => string");

AssertType("now", "string");
    return "now"
}

class X {
    static now() {
AssertType({}, "{}");
        return {
}
    }

    why() {

    }
}

class Y {

}

console.log(X.now()) // works as expected
AssertType(console.log(X.now()), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(X.now(), "{}");
AssertType(X.now, "() => {}");

console.log(Y.now()) // works as expected
AssertType(console.log(Y.now()), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(Y.now(), "string");
AssertType(Y.now, "() => string");

export const x: X | number = Math.random() > 0.5 ? new X() : 1
AssertType(x, "union");
AssertType(Math.random() > 0.5 ? new X() : 1, "union");
AssertType(Math.random() > 0.5, "boolean");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(0.5, "double");
AssertType(new X(), "X");
AssertType(X, "typeof X");
AssertType(1, "int");

if (x instanceof X) {
    x.why() // should compile
AssertType(x.why(), "void");
AssertType(x.why, "() => void");
}

