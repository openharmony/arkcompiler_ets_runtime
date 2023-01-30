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

// === tests/cases/compiler/capturedLetConstInLoop10.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    foo() {
        for (let x of [0]) {
AssertType(x, "number");
AssertType([0], "number[]");
AssertType(0, "int");

            let f = function() { 
AssertType(f, "() => number");
AssertType(function() { return x; }, "() => number");
AssertType(x, "number");
return x; };

            this.bar(f());
AssertType(this.bar(f()), "void");
AssertType(this.bar, "(number) => void");
AssertType(this, "this");
AssertType(f(), "number");
AssertType(f, "() => number");
        }
    }
    bar(a: number) {
    }

    baz() {
        for (let x of [1]) {
AssertType(x, "number");
AssertType([1], "number[]");
AssertType(1, "int");

            let a = function() {  
AssertType(a, "() => number");
AssertType(function() {  return x; }, "() => number");
AssertType(x, "number");
return x; };

            for (let y of [1]) {
AssertType(y, "number");
AssertType([1], "number[]");
AssertType(1, "int");

                let b = function() { 
AssertType(b, "() => number");
AssertType(function() { return y; }, "() => number");
AssertType(y, "number");
return y; };

                this.bar(b());
AssertType(this.bar(b()), "void");
AssertType(this.bar, "(number) => void");
AssertType(this, "this");
AssertType(b(), "number");
AssertType(b, "() => number");
            }
            this.bar(a());
AssertType(this.bar(a()), "void");
AssertType(this.bar, "(number) => void");
AssertType(this, "this");
AssertType(a(), "number");
AssertType(a, "() => number");
        }
    }
    baz2() {
        for (let x of [1]) {
AssertType(x, "number");
AssertType([1], "number[]");
AssertType(1, "int");

            let a = function() {  
AssertType(a, "() => number");
AssertType(function() {  return x; }, "() => number");
AssertType(x, "number");
return x; };

            this.bar(a());
AssertType(this.bar(a()), "void");
AssertType(this.bar, "(number) => void");
AssertType(this, "this");
AssertType(a(), "number");
AssertType(a, "() => number");

            for (let y of [1]) {
AssertType(y, "number");
AssertType([1], "number[]");
AssertType(1, "int");

                let b = function() { 
AssertType(b, "() => number");
AssertType(function() { return y; }, "() => number");
AssertType(y, "number");
return y; };

                this.bar(b());
AssertType(this.bar(b()), "void");
AssertType(this.bar, "(number) => void");
AssertType(this, "this");
AssertType(b(), "number");
AssertType(b, "() => number");
            }
        }
    }
}

class B {
    foo() {
        let a =
AssertType(a, "() => void");

            () => {
AssertType(() => {                for (let x of [0]) {                    let f = () => x;                    this.bar(f());                }            }, "() => void");

                for (let x of [0]) {
AssertType(x, "number");
AssertType([0], "number[]");
AssertType(0, "int");

                    let f = () => x;
AssertType(f, "() => number");
AssertType(() => x, "() => number");
AssertType(x, "number");

                    this.bar(f());
AssertType(this.bar(f()), "void");
AssertType(this.bar, "(number) => void");
AssertType(this, "this");
AssertType(f(), "number");
AssertType(f, "() => number");
                }
            }
    }
    bar(a: number) {
    }
}

