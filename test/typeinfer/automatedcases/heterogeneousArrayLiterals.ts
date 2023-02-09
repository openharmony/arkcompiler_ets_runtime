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

// === tests/cases/conformance/types/typeRelationships/bestCommonType/heterogeneousArrayLiterals.ts ===
declare function AssertType(value:any, type:string):void;
// type of an array is the best common type of its elements (plus its contextual type if it exists)

let a = [1, '']; // {}[]
AssertType(a, "(union)[]");
AssertType([1, ''], "(union)[]");
AssertType(1, "int");
AssertType('', "string");

let b = [1, null]; // number[]
AssertType(b, "number[]");
AssertType([1, null], "number[]");
AssertType(1, "int");
AssertType(null, "null");

let c = [1, '', null]; // {}[]
AssertType(c, "(union)[]");
AssertType([1, '', null], "(union)[]");
AssertType(1, "int");
AssertType('', "string");
AssertType(null, "null");

let d = [{}, 1]; // {}[]
AssertType(d, "{}[]");
AssertType([{}, 1], "(union)[]");
AssertType({}, "{}");
AssertType(1, "int");

let e = [{}, Object]; // {}[]
AssertType(e, "{}[]");
AssertType([{}, Object], "(union)[]");
AssertType({}, "{}");
AssertType(Object, "ObjectConstructor");

let f = [[], [1]]; // number[][]
AssertType(f, "number[][]");
AssertType([[], [1]], "number[][]");
AssertType([], "undefined[]");
AssertType([1], "number[]");
AssertType(1, "int");

let g = [[1], ['']]; // {}[]
AssertType(g, "(union)[]");
AssertType([[1], ['']], "(union)[]");
AssertType([1], "number[]");
AssertType(1, "int");
AssertType([''], "string[]");
AssertType('', "string");

let h = [{ foo: 1, bar: '' }, { foo: 2 }]; // {foo: number}[]
AssertType(h, "(union)[]");
AssertType([{ foo: 1, bar: '' }, { foo: 2 }], "(union)[]");
AssertType({ foo: 1, bar: '' }, "{ foo: number; bar: string; }");
AssertType(foo, "number");
AssertType(1, "int");
AssertType(bar, "string");
AssertType('', "string");
AssertType({ foo: 2 }, "{ foo: number; }");
AssertType(foo, "number");
AssertType(2, "int");

let i = [{ foo: 1, bar: '' }, { foo: '' }]; // {}[]
AssertType(i, "(union)[]");
AssertType([{ foo: 1, bar: '' }, { foo: '' }], "(union)[]");
AssertType({ foo: 1, bar: '' }, "{ foo: number; bar: string; }");
AssertType(foo, "number");
AssertType(1, "int");
AssertType(bar, "string");
AssertType('', "string");
AssertType({ foo: '' }, "{ foo: string; }");
AssertType(foo, "string");
AssertType('', "string");

let j = [() => 1, () => '']; // {}[]
AssertType(j, "(union)[]");
AssertType([() => 1, () => ''], "(union)[]");
AssertType(() => 1, "() => number");
AssertType(1, "int");
AssertType(() => '', "() => string");
AssertType('', "string");

let k = [() => 1, () => 1]; // { (): number }[]
AssertType(k, "(() => number)[]");
AssertType([() => 1, () => 1], "(() => number)[]");
AssertType(() => 1, "() => number");
AssertType(1, "int");
AssertType(() => 1, "() => number");
AssertType(1, "int");

let l = [() => 1, () => null]; // { (): any }[]
AssertType(l, "(() => any)[]");
AssertType([() => 1, () => null], "(() => any)[]");
AssertType(() => 1, "() => number");
AssertType(1, "int");
AssertType(() => null, "() => any");
AssertType(null, "null");

let m = [() => 1, () => '', () => null]; // { (): any }[]
AssertType(m, "(() => any)[]");
AssertType([() => 1, () => '', () => null], "(() => any)[]");
AssertType(() => 1, "() => number");
AssertType(1, "int");
AssertType(() => '', "() => string");
AssertType('', "string");
AssertType(() => null, "() => any");
AssertType(null, "null");

let n = [[() => 1], [() => '']]; // {}[]
AssertType(n, "(union)[]");
AssertType([[() => 1], [() => '']], "(union)[]");
AssertType([() => 1], "(() => number)[]");
AssertType(() => 1, "() => number");
AssertType(1, "int");
AssertType([() => ''], "(() => string)[]");
AssertType(() => '', "() => string");
AssertType('', "string");

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Base { baz: string; }
let base: Base;
AssertType(base, "Base");

let derived: Derived;
AssertType(derived, "Derived");

let derived2: Derived2;
AssertType(derived2, "Derived2");

module Derived {
    let h = [{ foo: base, basear: derived }, { foo: base }]; // {foo: Base}[]
    let i = [{ foo: base, basear: derived }, { foo: derived }]; // {foo: Derived}[]

    let j = [() => base, () => derived]; // { {}: Base }
    let k = [() => base, () => 1]; // {}[]~
    let l = [() => base, () => null]; // { (): any }[]
    let m = [() => base, () => derived, () => null]; // { (): any }[]
    let n = [[() => base], [() => derived]]; // { (): Base }[]
    let o = [derived, derived2]; // {}[]
    let p = [derived, derived2, base]; // Base[]
    let q = [[() => derived2], [() => derived]]; // {}[]
}

module WithContextualType {
    // no errors
    let a: Base[] = [derived, derived2];
    let b: Derived[] = [null];
    let c: Derived[] = [];
    let d: { (): Base }[] = [() => derived, () => derived2];
}

function foo<T, U>(t: T, u: U) {
    let a = [t, t]; // T[]
AssertType(a, "T[]");
AssertType([t, t], "T[]");
AssertType(t, "T");
AssertType(t, "T");

    let b = [t, null]; // T[]
AssertType(b, "T[]");
AssertType([t, null], "T[]");
AssertType(t, "T");
AssertType(null, "null");

    let c = [t, u]; // {}[]
AssertType(c, "(union)[]");
AssertType([t, u], "(union)[]");
AssertType(t, "T");
AssertType(u, "U");

    let d = [t, 1]; // {}[]
AssertType(d, "(union)[]");
AssertType([t, 1], "(union)[]");
AssertType(t, "T");
AssertType(1, "int");

    let e = [() => t, () => u]; // {}[]
AssertType(e, "(union)[]");
AssertType([() => t, () => u], "(union)[]");
AssertType(() => t, "() => T");
AssertType(t, "T");
AssertType(() => u, "() => U");
AssertType(u, "U");

    let f = [() => t, () => u, () => null]; // { (): any }[]
AssertType(f, "(() => any)[]");
AssertType([() => t, () => u, () => null], "(() => any)[]");
AssertType(() => t, "() => T");
AssertType(t, "T");
AssertType(() => u, "() => U");
AssertType(u, "U");
AssertType(() => null, "() => any");
AssertType(null, "null");
}

function foo2<T extends Base, U extends Derived>(t: T, u: U) {
    let a = [t, t]; // T[]
AssertType(a, "T[]");
AssertType([t, t], "T[]");
AssertType(t, "T");
AssertType(t, "T");

    let b = [t, null]; // T[]
AssertType(b, "T[]");
AssertType([t, null], "T[]");
AssertType(t, "T");
AssertType(null, "null");

    let c = [t, u]; // {}[]
AssertType(c, "(union)[]");
AssertType([t, u], "(union)[]");
AssertType(t, "T");
AssertType(u, "U");

    let d = [t, 1]; // {}[]
AssertType(d, "(union)[]");
AssertType([t, 1], "(union)[]");
AssertType(t, "T");
AssertType(1, "int");

    let e = [() => t, () => u]; // {}[]
AssertType(e, "(union)[]");
AssertType([() => t, () => u], "(union)[]");
AssertType(() => t, "() => T");
AssertType(t, "T");
AssertType(() => u, "() => U");
AssertType(u, "U");

    let f = [() => t, () => u, () => null]; // { (): any }[]
AssertType(f, "(() => any)[]");
AssertType([() => t, () => u, () => null], "(() => any)[]");
AssertType(() => t, "() => T");
AssertType(t, "T");
AssertType(() => u, "() => U");
AssertType(u, "U");
AssertType(() => null, "() => any");
AssertType(null, "null");

    let g = [t, base]; // Base[]
AssertType(g, "Base[]");
AssertType([t, base], "Base[]");
AssertType(t, "T");
AssertType(base, "Base");

    let h = [t, derived]; // Derived[]
AssertType(h, "(union)[]");
AssertType([t, derived], "(union)[]");
AssertType(t, "T");
AssertType(derived, "Derived");

    let i = [u, base]; // Base[]
AssertType(i, "Base[]");
AssertType([u, base], "Base[]");
AssertType(u, "U");
AssertType(base, "Base");

    let j = [u, derived]; // Derived[]
AssertType(j, "Derived[]");
AssertType([u, derived], "Derived[]");
AssertType(u, "U");
AssertType(derived, "Derived");
}

function foo3<T extends Derived, U extends Derived>(t: T, u: U) {
    let a = [t, t]; // T[]
AssertType(a, "T[]");
AssertType([t, t], "T[]");
AssertType(t, "T");
AssertType(t, "T");

    let b = [t, null]; // T[]
AssertType(b, "T[]");
AssertType([t, null], "T[]");
AssertType(t, "T");
AssertType(null, "null");

    let c = [t, u]; // {}[]
AssertType(c, "(union)[]");
AssertType([t, u], "(union)[]");
AssertType(t, "T");
AssertType(u, "U");

    let d = [t, 1]; // {}[]
AssertType(d, "(union)[]");
AssertType([t, 1], "(union)[]");
AssertType(t, "T");
AssertType(1, "int");

    let e = [() => t, () => u]; // {}[]
AssertType(e, "(union)[]");
AssertType([() => t, () => u], "(union)[]");
AssertType(() => t, "() => T");
AssertType(t, "T");
AssertType(() => u, "() => U");
AssertType(u, "U");

    let f = [() => t, () => u, () => null]; // { (): any }[]
AssertType(f, "(() => any)[]");
AssertType([() => t, () => u, () => null], "(() => any)[]");
AssertType(() => t, "() => T");
AssertType(t, "T");
AssertType(() => u, "() => U");
AssertType(u, "U");
AssertType(() => null, "() => any");
AssertType(null, "null");

    let g = [t, base]; // Base[]
AssertType(g, "Base[]");
AssertType([t, base], "Base[]");
AssertType(t, "T");
AssertType(base, "Base");

    let h = [t, derived]; // Derived[]
AssertType(h, "Derived[]");
AssertType([t, derived], "Derived[]");
AssertType(t, "T");
AssertType(derived, "Derived");

    let i = [u, base]; // Base[]
AssertType(i, "Base[]");
AssertType([u, base], "Base[]");
AssertType(u, "U");
AssertType(base, "Base");

    let j = [u, derived]; // Derived[]
AssertType(j, "Derived[]");
AssertType([u, derived], "Derived[]");
AssertType(u, "U");
AssertType(derived, "Derived");
}

function foo4<T extends Base, U extends Base>(t: T, u: U) {
    let a = [t, t]; // T[]
AssertType(a, "T[]");
AssertType([t, t], "T[]");
AssertType(t, "T");
AssertType(t, "T");

    let b = [t, null]; // T[]
AssertType(b, "T[]");
AssertType([t, null], "T[]");
AssertType(t, "T");
AssertType(null, "null");

    let c = [t, u]; // BUG 821629
AssertType(c, "(union)[]");
AssertType([t, u], "(union)[]");
AssertType(t, "T");
AssertType(u, "U");

    let d = [t, 1]; // {}[]
AssertType(d, "(union)[]");
AssertType([t, 1], "(union)[]");
AssertType(t, "T");
AssertType(1, "int");

    let e = [() => t, () => u]; // {}[]
AssertType(e, "(union)[]");
AssertType([() => t, () => u], "(union)[]");
AssertType(() => t, "() => T");
AssertType(t, "T");
AssertType(() => u, "() => U");
AssertType(u, "U");

    let f = [() => t, () => u, () => null]; // { (): any }[]
AssertType(f, "(() => any)[]");
AssertType([() => t, () => u, () => null], "(() => any)[]");
AssertType(() => t, "() => T");
AssertType(t, "T");
AssertType(() => u, "() => U");
AssertType(u, "U");
AssertType(() => null, "() => any");
AssertType(null, "null");

    let g = [t, base]; // Base[]
AssertType(g, "Base[]");
AssertType([t, base], "Base[]");
AssertType(t, "T");
AssertType(base, "Base");

    let h = [t, derived]; // Derived[]
AssertType(h, "(union)[]");
AssertType([t, derived], "(union)[]");
AssertType(t, "T");
AssertType(derived, "Derived");

    let i = [u, base]; // Base[]
AssertType(i, "Base[]");
AssertType([u, base], "Base[]");
AssertType(u, "U");
AssertType(base, "Base");

    let j = [u, derived]; // Derived[]
AssertType(j, "(union)[]");
AssertType([u, derived], "(union)[]");
AssertType(u, "U");
AssertType(derived, "Derived");

    let k: Base[] = [t, u];
AssertType(k, "Base[]");
AssertType([t, u], "(union)[]");
AssertType(t, "T");
AssertType(u, "U");
}

//function foo3<T extends U, U extends Derived>(t: T, u: U) {
//    let a = [t, t]; // T[]
//    let b = [t, null]; // T[]
//    let c = [t, u]; // {}[]
//    let d = [t, 1]; // {}[]
//    let e = [() => t, () => u]; // {}[]
//    let f = [() => t, () => u, () => null]; // { (): any }[]

//    let g = [t, base]; // Base[]
//    let h = [t, derived]; // Derived[]
//    let i = [u, base]; // Base[]
//    let j = [u, derived]; // Derived[]
//}

//function foo4<T extends U, U extends Base>(t: T, u: U) {
//    let a = [t, t]; // T[]
//    let b = [t, null]; // T[]
//    let c = [t, u]; // BUG 821629
//    let d = [t, 1]; // {}[]
//    let e = [() => t, () => u]; // {}[]
//    let f = [() => t, () => u, () => null]; // { (): any }[]

//    let g = [t, base]; // Base[]
//    let h = [t, derived]; // Derived[]
//    let i = [u, base]; // Base[]
//    let j = [u, derived]; // Derived[]

//    let k: Base[] = [t, u];
//}

