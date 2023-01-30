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
export class A {}

export class B {
    static cat = "cat";
}

export class C {
    static Cls = class {}
}

export class D {
    /**
     * @param {number} a
     * @param {number} b
     */
    constructor(a, b) {}
}

/**
 * @template T,U
 */
export class E {
    /**
     * @type {T & U}
     */
    field;

    // @readonly is currently unsupported, it seems - included here just in case that changes
    /**
     * @type {T & U}
     * @readonly
     */
    readonlyField;

    initializedField = 12;

    /**
     * @
return {U}
     */
    get f1() { 
AssertType((null), "any");
return /** @type {*} */(null); 

AssertType(null, "null");
}

    /**
     * @param {U} _p
     */
    set f1(_p) {}

    /**
     * @
return {U}
     */
    get f2() { 
AssertType((null), "any");
return /** @type {*} */(null); 

AssertType(null, "null");
}

    /**
     * @param {U} _p
     */
    set f3(_p) {}

    /**
     * @param {T} a
     * @param {U} b
     */
    constructor(a, b) {}


    /**
     * @type {string}
     */
    static staticField;

    // @readonly is currently unsupported, it seems - included here just in case that changes
    /**
     * @type {string}
     * @readonly
     */
    static staticReadonlyField;

    static staticInitializedField = 12;

    /**
     * @
return {string}
     */
    static get s1() { 
AssertType("", "string");
return ""; 
}

    /**
     * @param {string} _p
     */
    static set s1(_p) {}

    /**
     * @
return {string}
     */
    static get s2() { 
AssertType("", "string");
return ""; 
}

    /**
     * @param {string} _p
     */
    static set s3(_p) {}
}

/**
 * @template T,U
 */
export class F {
    /**
     * @type {T & U}
     */
    field;
    /**
     * @param {T} a
     * @param {U} b
     */
    constructor(a, b) {}

    /**
     * @template A,B
     * @param {A} a
     * @param {B} b
     */
    static create(a, b) { 
AssertType(new F(a, b), "F<A, B>");
return new F(a, b); 

AssertType(F, "typeof F");

AssertType(a, "A");

AssertType(b, "B");
}
}

class G {}

export { G };

class HH {}

export { HH as H };

export class I {}
export { I as II };

export { J as JJ };
export class J {}


export class K {
    constructor() {
        this.p1 = 12;
AssertType(this.p1 = 12, "int");
AssertType(this.p1, "any");
AssertType(this, "this");
AssertType(12, "int");

        this.p2 = "ok";
AssertType(this.p2 = "ok", "string");
AssertType(this.p2, "any");
AssertType(this, "this");
AssertType("ok", "string");
    }

    method() {
AssertType(this.p1, "number");
AssertType(this, "this");
        return this.p1;
    }
}

export class L extends K {}

export class M extends null {
    constructor() {
        this.prop = 12;
AssertType(this.prop = 12, "int");
AssertType(this.prop, "any");
AssertType(this, "this");
AssertType(12, "int");
    }
}


/**
 * @template T
 */
export class N extends L {
    /**
     * @param {T} param 
     */
    constructor(param) {
        super();
AssertType(super(), "void");
AssertType(super, "typeof L");

        this.another = param;
AssertType(this.another = param, "T");
AssertType(this.another, "any");
AssertType(this, "this");
AssertType(param, "T");
    }
}

/**
 * @template U
 * @extends {N<U>}
 */
export class O extends N {
    /**
     * @param {U} param 
     */
    constructor(param) {
        super(param);
AssertType(super(param), "void");
AssertType(super, "typeof N");
AssertType(param, "U");

        this.another2 = param;
AssertType(this.another2 = param, "U");
AssertType(this.another2, "any");
AssertType(this, "this");
AssertType(param, "U");
    }
}

let x = /** @type {*} */(null);
AssertType(x, "any");
AssertType((null), "any");
AssertType(null, "null");

export class VariableBase extends x {}

export class HasStatics {
    static staticMethod() {}
}

export class ExtendsStatics extends HasStatics {
    static also() {}
}


