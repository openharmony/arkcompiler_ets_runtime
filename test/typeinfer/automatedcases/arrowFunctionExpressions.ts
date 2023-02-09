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

// === tests/cases/conformance/expressions/functions/arrowFunctionExpressions.ts ===
declare function AssertType(value:any, type:string):void;
// ArrowFormalParameters => AssignmentExpression is equivalent to ArrowFormalParameters => { return AssignmentExpression; }
let a = (p: string) => p.length;
AssertType(a, "(string) => number");
AssertType((p: string) => p.length, "(string) => number");
AssertType(p, "string");
AssertType(p.length, "number");

let a = (p: string) => { 
AssertType(a, "(string) => number");
return p.length; 

AssertType((p: string) => { return p.length; }, "(string) => number");

AssertType(p, "string");

AssertType(p.length, "number");
}

// Identifier => Block is equivalent to(Identifier) => Block
let b = j => { 
AssertType(b, "(any) => number");
return 0; 

AssertType(j => { return 0; }, "(any) => number");

AssertType(j, "any");

AssertType(0, "int");
}

let b = (j) => { 
AssertType(b, "(any) => number");
return 0; 

AssertType((j) => { return 0; }, "(any) => number");

AssertType(j, "any");

AssertType(0, "int");
}

// Identifier => AssignmentExpression is equivalent to(Identifier) => AssignmentExpression
let c: number;
AssertType(c, "number");

let d = n => c = n;
AssertType(d, "(any) => any");
AssertType(n => c = n, "(any) => any");
AssertType(n, "any");
AssertType(c = n, "any");
AssertType(c, "number");
AssertType(n, "any");

let d = (n) => c = n;
AssertType(d, "(any) => any");
AssertType((n) => c = n, "(any) => any");
AssertType(n, "any");
AssertType(c = n, "any");
AssertType(c, "number");
AssertType(n, "any");

let d: (n: any) => any;
AssertType(d, "(any) => any");
AssertType(n, "any");

// Binding patterns in arrow functions
let p1 = ([a]) => { };
AssertType(p1, "([any]) => void");
AssertType(([a]) => { }, "([any]) => void");
AssertType(a, "any");

let p2 = ([...a]) => { };
AssertType(p2, "(any[]) => void");
AssertType(([...a]) => { }, "(any[]) => void");
AssertType(a, "any[]");

let p3 = ([, a]) => { };
AssertType(p3, "([any, any]) => void");
AssertType(([, a]) => { }, "([any, any]) => void");
AssertType(, "undefined");
AssertType(a, "any");

let p4 = ([, ...a]) => { };
AssertType(p4, "([any?, ...any[]]) => void");
AssertType(([, ...a]) => { }, "([any?, ...any[]]) => void");
AssertType(, "undefined");
AssertType(a, "any[]");

let p5 = ([a = 1]) => { };
AssertType(p5, "([number?]) => void");
AssertType(([a = 1]) => { }, "([number?]) => void");
AssertType(a, "number");
AssertType(1, "int");

let p6 = ({ a }) => { };
AssertType(p6, "({ a: any; }) => void");
AssertType(({ a }) => { }, "({ a: any; }) => void");
AssertType(a, "any");

let p7 = ({ a: { b } }) => { };
AssertType(p7, "({ a: { b: any; }; }) => void");
AssertType(({ a: { b } }) => { }, "({ a: { b: any; }; }) => void");
AssertType(a, "any");
AssertType(b, "any");

let p8 = ({ a = 1 }) => { };
AssertType(p8, "({ a?: number; }) => void");
AssertType(({ a = 1 }) => { }, "({ a?: number; }) => void");
AssertType(a, "number");
AssertType(1, "int");

let p9 = ({ a: { b = 1 } = { b: 1 } }) => { };
AssertType(p9, "({ a?: { b?: number; }; }) => void");
AssertType(({ a: { b = 1 } = { b: 1 } }) => { }, "({ a?: { b?: number; }; }) => void");
AssertType(a, "any");
AssertType(b, "number");
AssertType(1, "int");
AssertType({ b: 1 }, "{ b?: number; }");
AssertType(b, "number");
AssertType(1, "int");

let p10 = ([{ value, done }]) => { };
AssertType(p10, "([{ value: any; done: any; }]) => void");
AssertType(([{ value, done }]) => { }, "([{ value: any; done: any; }]) => void");
AssertType(value, "any");
AssertType(done, "any");

// Arrow function used in class member initializer
// Arrow function used in class member function
class MyClass {
    m = (n) => n + 1;
    p = (n) => n && this;

    fn() {
        let m = (n) => n + 1;
AssertType(m, "(any) => any");
AssertType((n) => n + 1, "(any) => any");
AssertType(n, "any");
AssertType(n + 1, "any");
AssertType(n, "any");
AssertType(1, "int");

        let p = (n) => n && this;
AssertType(p, "(any) => this");
AssertType((n) => n && this, "(any) => this");
AssertType(n, "any");
AssertType(n && this, "this");
AssertType(n, "any");
AssertType(this, "this");
    }
}

// Arrow function used in arrow function
let arrrr = () => (m: number) => () => (n: number) => m + n;
AssertType(arrrr, "() => (number) => () => (number) => number");
AssertType(() => (m: number) => () => (n: number) => m + n, "() => (number) => () => (number) => number");
AssertType((m: number) => () => (n: number) => m + n, "(number) => () => (number) => number");
AssertType(m, "number");
AssertType(() => (n: number) => m + n, "() => (number) => number");
AssertType((n: number) => m + n, "(number) => number");
AssertType(n, "number");
AssertType(m + n, "number");
AssertType(m, "number");
AssertType(n, "number");

let e = arrrr()(3)()(4);
AssertType(e, "number");
AssertType(arrrr()(3)()(4), "number");
AssertType(arrrr()(3)(), "(number) => number");
AssertType(arrrr()(3), "() => (number) => number");
AssertType(arrrr(), "(number) => () => (number) => number");
AssertType(arrrr, "() => (number) => () => (number) => number");
AssertType(3, "int");
AssertType(4, "int");

let e: number;
AssertType(e, "number");

// Arrow function used in arrow function used in function
function someFn() {
    let arr = (n: number) => (p: number) => p * n;
AssertType(arr, "(number) => (number) => number");
AssertType((n: number) => (p: number) => p * n, "(number) => (number) => number");
AssertType(n, "number");
AssertType((p: number) => p * n, "(number) => number");
AssertType(p, "number");
AssertType(p * n, "number");
AssertType(p, "number");
AssertType(n, "number");

    arr(3)(4).toExponential();
AssertType(arr(3)(4).toExponential(), "string");
AssertType(arr(3)(4).toExponential, "(?number) => string");
AssertType(arr(3)(4), "number");
AssertType(arr(3), "(number) => number");
AssertType(arr, "(number) => (number) => number");
AssertType(3, "int");
AssertType(4, "int");
}

// Arrow function used in function
function someOtherFn() {
    let arr = (n: number) => '' + n;
AssertType(arr, "(number) => string");
AssertType((n: number) => '' + n, "(number) => string");
AssertType(n, "number");
AssertType('' + n, "string");
AssertType('', "string");
AssertType(n, "number");

    arr(4).charAt(0);
AssertType(arr(4).charAt(0), "string");
AssertType(arr(4).charAt, "(number) => string");
AssertType(arr(4), "string");
AssertType(arr, "(number) => string");
AssertType(4, "int");
AssertType(0, "int");
}

// Arrow function used in nested function in function
function outerFn() {
    function innerFn() {
AssertType(innerFn, "() => void");

        let arrowFn = () => { };
AssertType(arrowFn, "() => void");
AssertType(() => { }, "() => void");

        let p = arrowFn();
AssertType(p, "void");
AssertType(arrowFn(), "void");
AssertType(arrowFn, "() => void");

        let p: void;
AssertType(p, "void");
    }
}

// Arrow function used in nested function in arrow function
let f = (n: string) => {
AssertType(f, "(string) => () => string");
AssertType(n, "string");
AssertType((n: string) => {    function fn(x: number) {        return () => n + x;    }    return fn(4);}, "(string) => () => string");

    function fn(x: number) {
AssertType(fn, "(number) => () => string");
AssertType(x, "number");

AssertType(() => n + x, "() => string");
AssertType(n + x, "string");
AssertType(n, "string");
AssertType(x, "number");
        return () => n + x;
    }
AssertType(fn(4), "() => string");
AssertType(fn, "(number) => () => string");
AssertType(4, "int");
    return fn(4);
}
let g = f('')();
AssertType(g, "string");
AssertType(f('')(), "string");
AssertType(f(''), "() => string");
AssertType(f, "(string) => () => string");
AssertType('', "string");

let g: string;
AssertType(g, "string");


// Arrow function used in nested function in arrow function in nested function
function someOuterFn() {
    let arr = (n: string) => {
AssertType(arr, "(string) => () => () => number");
AssertType(n, "string");
AssertType((n: string) => {        function innerFn() {            return () => n.length;        }        return innerFn;    }, "(string) => () => () => number");

        function innerFn() {
AssertType(innerFn, "() => () => number");

AssertType(() => n.length, "() => number");
AssertType(n.length, "number");
            return () => n.length;
        }
AssertType(innerFn, "() => () => number");
        return innerFn;
    }
AssertType(arr, "(string) => () => () => number");
    return arr;
}
let h = someOuterFn()('')()();
AssertType(h, "number");
AssertType(someOuterFn()('')()(), "number");
AssertType(someOuterFn()('')(), "() => number");
AssertType(someOuterFn()(''), "() => () => number");
AssertType(someOuterFn(), "(string) => () => () => number");
AssertType(someOuterFn, "() => (string) => () => () => number");
AssertType('', "string");

h.toExponential();
AssertType(h.toExponential(), "string");
AssertType(h.toExponential, "(?number) => string");

// Arrow function used in try/catch/finally in function
function tryCatchFn() {
    try {
        let x = () => this;
AssertType(x, "() => any");
AssertType(() => this, "() => any");
AssertType(this, "any");

    } catch (e) {
AssertType(e, "any");

        let t = () => e + this;
AssertType(t, "() => any");
AssertType(() => e + this, "() => any");
AssertType(e + this, "any");
AssertType(e, "any");
AssertType(this, "any");

    } finally {
        let m = () => this + '';
AssertType(m, "() => string");
AssertType(() => this + '', "() => string");
AssertType(this + '', "string");
AssertType(this, "any");
AssertType('', "string");
    }
}


