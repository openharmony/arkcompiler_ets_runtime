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

// === tests/cases/conformance/classes/members/privateNames/privateNameStaticMethodCallExpression.ts ===
declare function AssertType(value:any, type:string):void;
class AA {
    static #method() { this.x = 10; };
AssertType(this.x = 10, "int");
AssertType(this.x, "number");
AssertType(this, "typeof AA");
AssertType(10, "int");

    static #method2(a, ...b) {};
    static x = 1;
    test() {
        AA.#method();
AssertType(AA.#method(), "void");
AssertType(AA.#method, "() => void");

        const func = AA.#method;
AssertType(func, "() => void");
AssertType(AA.#method, "() => void");

        func();
AssertType(func(), "void");
AssertType(func, "() => void");

        new AA.#method();
AssertType(new AA.#method(), "any");
AssertType(AA.#method, "() => void");

        const arr = [ 1, 2 ];
AssertType(arr, "number[]");
AssertType([ 1, 2 ], "number[]");
AssertType(1, "int");
AssertType(2, "int");

        AA.#method2(0, ...arr, 3);
AssertType(AA.#method2(0, ...arr, 3), "void");
AssertType(AA.#method2, "(any, ...any[]) => void");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const b = new AA.#method2(0, ...arr, 3); //Error 
AssertType(b, "any");
AssertType(new AA.#method2(0, ...arr, 3), "any");
AssertType(AA.#method2, "(any, ...any[]) => void");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const str = AA.#method2`head${1}middle${2}tail`;
AssertType(str, "void");
AssertType(AA.#method2`head${1}middle${2}tail`, "void");
AssertType(AA.#method2, "(any, ...any[]) => void");
AssertType(`head${1}middle${2}tail`, "string");
AssertType(1, "int");
AssertType(2, "int");

        AA.getClass().#method2`test${1}and${2}`;
AssertType(AA.getClass().#method2`test${1}and${2}`, "void");
AssertType(AA.getClass().#method2, "(any, ...any[]) => void");
AssertType(AA.getClass(), "typeof AA");
AssertType(AA.getClass, "() => typeof AA");
AssertType(`test${1}and${2}`, "string");
AssertType(1, "int");
AssertType(2, "int");

        AA.getClass().#method2(0, ...arr, 3); 
AssertType(AA.getClass().#method2(0, ...arr, 3), "void");
AssertType(AA.getClass().#method2, "(any, ...any[]) => void");
AssertType(AA.getClass(), "typeof AA");
AssertType(AA.getClass, "() => typeof AA");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const b2 = new (AA.getClass().#method2)(0, ...arr, 3); //Error 
AssertType(b2, "any");
AssertType(new (AA.getClass().#method2)(0, ...arr, 3), "any");
AssertType((AA.getClass().#method2), "(any, ...any[]) => void");
AssertType(AA.getClass().#method2, "(any, ...any[]) => void");
AssertType(AA.getClass(), "typeof AA");
AssertType(AA.getClass, "() => typeof AA");
AssertType(0, "int");
AssertType(...arr, "number");
AssertType(arr, "number[]");
AssertType(3, "int");

        const str2 = AA.getClass().#method2`head${1}middle${2}tail`;
AssertType(str2, "void");
AssertType(AA.getClass().#method2`head${1}middle${2}tail`, "void");
AssertType(AA.getClass().#method2, "(any, ...any[]) => void");
AssertType(AA.getClass(), "typeof AA");
AssertType(AA.getClass, "() => typeof AA");
AssertType(`head${1}middle${2}tail`, "string");
AssertType(1, "int");
AssertType(2, "int");
    }
    static getClass() { 
AssertType(AA, "typeof AA");
return AA; 
}
}


