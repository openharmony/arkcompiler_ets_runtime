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

// === tests/cases/conformance/jsdoc/declarations/index.js ===
declare function AssertType(value:any, type:string):void;
export class A {
    get x() {
AssertType(12, "int");
        return 12;
    }
}

export class B {
    /**
     * @param {number} _arg
     */
    set x(_arg) {
    }
}

export class C {
    get x() {
AssertType(12, "int");
        return 12;
    }
    set x(_arg) {
    }
}

export class D {}
Object.defineProperty(D.prototype, "x", {
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(D.prototype, "D");
AssertType("x", "string");
AssertType({    get() {        return 12;    }}, "{ get(): number; }");
AssertType(Object.defineProperty(D.prototype, "x", {    get() {        return 12;    }}), "D");

    get() {
AssertType(get, "() => number");

AssertType(12, "int");
        return 12;
    }
});

export class E {}
Object.defineProperty(E.prototype, "x", {
AssertType(Object.defineProperty(E.prototype, "x", {    /**     * @param {number} _arg     */    set(_arg) {}}), "E");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(E.prototype, "E");
AssertType("x", "string");
AssertType({    /**     * @param {number} _arg     */    set(_arg) {}}, "{ set(number): void; }");

    /**
     * @param {number} _arg
     */
    set(_arg) {
AssertType(set, "(number) => void");

AssertType(_arg, "number");
}

});

export class F {}
Object.defineProperty(F.prototype, "x", {
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(F.prototype, "F");
AssertType("x", "string");
AssertType({    get() {        return 12;    },    /**     * @param {number} _arg     */    set(_arg) {}}, "{ get(): number; set(number): void; }");
AssertType(Object.defineProperty(F.prototype, "x", {    get() {        return 12;    },    /**     * @param {number} _arg     */    set(_arg) {}}), "F");

    get() {
AssertType(get, "() => number");

AssertType(12, "int");
        return 12;

    },
    /**
     * @param {number} _arg
     */
    set(_arg) {
AssertType(set, "(number) => void");

AssertType(_arg, "number");
}

});


