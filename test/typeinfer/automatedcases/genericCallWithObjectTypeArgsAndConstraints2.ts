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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithObjectTypeArgsAndConstraints2.ts ===
declare function AssertType(value:any, type:string):void;
// Generic call with constraints infering type parameter from object member properties
// No errors expected

class Base {
    x: string;
}
class Derived extends Base {
    y: string;
}

function f<T extends Base>(x: { foo: T; bar: T }) {
    let r: T;
AssertType(r, "T");

AssertType(r, "T");
    return r;
}
let r = f({ foo: new Base(), bar: new Derived() });
AssertType(r, "Base");
AssertType(f({ foo: new Base(), bar: new Derived() }), "Base");
AssertType(f, "<T extends Base>({ foo: T; bar: T; }) => T");
AssertType({ foo: new Base(), bar: new Derived() }, "{ foo: Base; bar: Derived; }");
AssertType(foo, "Base");
AssertType(new Base(), "Base");
AssertType(Base, "typeof Base");
AssertType(bar, "Derived");
AssertType(new Derived(), "Derived");
AssertType(Derived, "typeof Derived");

let r2 = f({ foo: new Derived(), bar: new Derived() });
AssertType(r2, "Derived");
AssertType(f({ foo: new Derived(), bar: new Derived() }), "Derived");
AssertType(f, "<T extends Base>({ foo: T; bar: T; }) => T");
AssertType({ foo: new Derived(), bar: new Derived() }, "{ foo: Derived; bar: Derived; }");
AssertType(foo, "Derived");
AssertType(new Derived(), "Derived");
AssertType(Derived, "typeof Derived");
AssertType(bar, "Derived");
AssertType(new Derived(), "Derived");
AssertType(Derived, "typeof Derived");


interface I<T> {
    a: T;
}
function f2<T extends Base>(x: I<T>) {
    let r: T;
AssertType(r, "T");

AssertType(r, "T");
    return r;
}
let i: I<Derived>;
AssertType(i, "I<Derived>");

let r3 = f2(i);
AssertType(r3, "Derived");
AssertType(f2(i), "Derived");
AssertType(f2, "<T extends Base>(I<T>) => T");
AssertType(i, "I<Derived>");


function f3<T extends Base>(x: T, y: (a: T) => T) {
AssertType(y(null), "T");
AssertType(y, "(T) => T");
AssertType(null, "null");
    return y(null);
}
let r4 = f3(new Base(), x => x);
AssertType(r4, "Base");
AssertType(f3(new Base(), x => x), "Base");
AssertType(f3, "<T extends Base>(T, (T) => T) => T");
AssertType(new Base(), "Base");
AssertType(Base, "typeof Base");
AssertType(x => x, "(Base) => Base");
AssertType(x, "Base");
AssertType(x, "Base");

let r5 = f3(new Derived(), x => x);
AssertType(r5, "Derived");
AssertType(f3(new Derived(), x => x), "Derived");
AssertType(f3, "<T extends Base>(T, (T) => T) => T");
AssertType(new Derived(), "Derived");
AssertType(Derived, "typeof Derived");
AssertType(x => x, "(Derived) => Derived");
AssertType(x, "Derived");
AssertType(x, "Derived");

let r6 = f3(null, null); // any
AssertType(r6, "any");
AssertType(f3(null, null), "any");
AssertType(f3, "<T extends Base>(T, (T) => T) => T");
AssertType(null, "null");
AssertType(null, "null");

let r7 = f3(null, x => x); // any
AssertType(r7, "any");
AssertType(f3(null, x => x), "any");
AssertType(f3, "<T extends Base>(T, (T) => T) => T");
AssertType(null, "null");
AssertType(x => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");


