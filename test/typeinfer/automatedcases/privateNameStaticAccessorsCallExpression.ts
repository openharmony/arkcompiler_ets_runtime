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

// === tests/cases/conformance/classes/members/privateNames/privateNameStaticAccessorsCallExpression.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    static get #fieldFunc() {  
AssertType(function() { A.#x = 10; }, "() => void");
return function() { A.#x = 10; } 

AssertType(A.#x = 10, "int");

AssertType(A.#x, "number");

AssertType(10, "int");
}

    static get #fieldFunc2() { 
AssertType(function(a, ...b) {}, "(any, ...any[]) => void");
return  function(a, ...b) {}; 

AssertType(a, "any");

AssertType(b, "any[]");
}

    static #x = 1;
    static test() {
        this.#fieldFunc();
AssertType(this.#fieldFunc(), "void");
AssertType(this.#fieldFunc, "() => void");
AssertType(this, "typeof A");

        const func = this.#fieldFunc;
AssertType(func, "() => void");
AssertType(this.#fieldFunc, "() => void");
AssertType(this, "typeof A");

        func();
AssertType(func(), "void");
AssertType(func, "() => void");

        new this.#fieldFunc();
AssertType(new this.#fieldFunc(), "any");
AssertType(this.#fieldFunc, "() => void");
AssertType(this, "typeof A");

        const arr = [ 1, 2 ];
AssertType(arr, "number[]");
AssertType([ 1, 2 ], "number[]");
AssertType(1, "int");
AssertType(2, "int");

        this.#fieldFunc2(0, ...arr, 3);
AssertType(this.#fieldFunc2(0, ...arr, 3), "void");
AssertType(this.#fieldFunc2, "(any, ...any[]) => void");
AssertType(this, "typeof A");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const b = new this.#fieldFunc2(0, ...arr, 3);
AssertType(b, "any");
AssertType(new this.#fieldFunc2(0, ...arr, 3), "any");
AssertType(this.#fieldFunc2, "(any, ...any[]) => void");
AssertType(this, "typeof A");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const str = this.#fieldFunc2`head${1}middle${2}tail`;
AssertType(str, "void");
AssertType(this.#fieldFunc2`head${1}middle${2}tail`, "void");
AssertType(this.#fieldFunc2, "(any, ...any[]) => void");
AssertType(this, "typeof A");
AssertType(`head${1}middle${2}tail`, "string");
AssertType(1, "int");
AssertType(2, "int");

        this.getClass().#fieldFunc2`test${1}and${2}`;
AssertType(this.getClass().#fieldFunc2`test${1}and${2}`, "void");
AssertType(this.getClass().#fieldFunc2, "(any, ...any[]) => void");
AssertType(this.getClass(), "typeof A");
AssertType(this.getClass, "() => typeof A");
AssertType(this, "typeof A");
AssertType(`test${1}and${2}`, "string");
AssertType(1, "int");
AssertType(2, "int");
    }
    static getClass() { 
AssertType(A, "typeof A");
return A; 
}
}

