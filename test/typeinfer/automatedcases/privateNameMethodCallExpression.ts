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

// === tests/cases/conformance/classes/members/privateNames/privateNameMethodCallExpression.ts ===
declare function AssertType(value:any, type:string):void;
class AA {
    #method() { this.x = 10; };
AssertType(this.x = 10, "int");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(10, "int");

    #method2(a, ...b) {};
    x = 1;
    test() {
        this.#method();
AssertType(this.#method(), "void");
AssertType(this.#method, "() => void");
AssertType(this, "this");

        const func = this.#method;
AssertType(func, "() => void");
AssertType(this.#method, "() => void");
AssertType(this, "this");

        func();
AssertType(func(), "void");
AssertType(func, "() => void");

        new this.#method();
AssertType(new this.#method(), "any");
AssertType(this.#method, "() => void");
AssertType(this, "this");

        const arr = [ 1, 2 ];
AssertType(arr, "number[]");
AssertType([ 1, 2 ], "number[]");
AssertType(1, "int");
AssertType(2, "int");

        this.#method2(0, ...arr, 3);
AssertType(this.#method2(0, ...arr, 3), "void");
AssertType(this.#method2, "(any, ...any[]) => void");
AssertType(this, "this");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const b = new this.#method2(0, ...arr, 3); //Error 
AssertType(b, "any");
AssertType(new this.#method2(0, ...arr, 3), "any");
AssertType(this.#method2, "(any, ...any[]) => void");
AssertType(this, "this");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const str = this.#method2`head${1}middle${2}tail`;
AssertType(str, "void");
AssertType(this.#method2`head${1}middle${2}tail`, "void");
AssertType(this.#method2, "(any, ...any[]) => void");
AssertType(this, "this");
AssertType(`head${1}middle${2}tail`, "string");
AssertType(1, "int");
AssertType(2, "int");

        this.getInstance().#method2`test${1}and${2}`;
AssertType(this.getInstance().#method2`test${1}and${2}`, "void");
AssertType(this.getInstance().#method2, "(any, ...any[]) => void");
AssertType(this.getInstance(), "AA");
AssertType(this.getInstance, "() => AA");
AssertType(this, "this");
AssertType(`test${1}and${2}`, "string");
AssertType(1, "int");
AssertType(2, "int");

        this.getInstance().#method2(0, ...arr, 3); 
AssertType(this.getInstance().#method2(0, ...arr, 3), "void");
AssertType(this.getInstance().#method2, "(any, ...any[]) => void");
AssertType(this.getInstance(), "AA");
AssertType(this.getInstance, "() => AA");
AssertType(this, "this");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const b2 = new (this.getInstance().#method2)(0, ...arr, 3); //Error 
AssertType(b2, "any");
AssertType(new (this.getInstance().#method2)(0, ...arr, 3), "any");
AssertType((this.getInstance().#method2), "(any, ...any[]) => void");
AssertType(this.getInstance().#method2, "(any, ...any[]) => void");
AssertType(this.getInstance(), "AA");
AssertType(this.getInstance, "() => AA");
AssertType(this, "this");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const str2 = this.getInstance().#method2`head${1}middle${2}tail`;
AssertType(str2, "void");
AssertType(this.getInstance().#method2`head${1}middle${2}tail`, "void");
AssertType(this.getInstance().#method2, "(any, ...any[]) => void");
AssertType(this.getInstance(), "AA");
AssertType(this.getInstance, "() => AA");
AssertType(this, "this");
AssertType(`head${1}middle${2}tail`, "string");
AssertType(1, "int");
AssertType(2, "int");
    }
    getInstance() { 
AssertType(new AA(), "AA");
return new AA(); 

AssertType(AA, "typeof AA");
}
}


