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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithObjectTypeArgs2.ts ===
declare function AssertType(value:any, type:string):void;
class Base {
    x: string;
}
class Derived extends Base {
    y: string;
}
class Derived2 extends Base {
    z: string;
}

// returns {}[]
function f<T extends Base, U extends Base>(a: { x: T; y: U }) {
AssertType([a.x, a.y], "(union)[]");
AssertType(a.x, "T");
AssertType(a.y, "U");
    return [a.x, a.y];
}

let r = f({ x: new Derived(), y: new Derived2() }); // {}[]
AssertType(r, "(union)[]");
AssertType(f({ x: new Derived(), y: new Derived2() }), "(union)[]");
AssertType(f, "<T extends Base, U extends Base>({ x: T; y: U; }) => (union)[]");
AssertType({ x: new Derived(), y: new Derived2() }, "{ x: Derived; y: Derived2; }");
AssertType(x, "Derived");
AssertType(new Derived(), "Derived");
AssertType(Derived, "typeof Derived");
AssertType(y, "Derived2");
AssertType(new Derived2(), "Derived2");
AssertType(Derived2, "typeof Derived2");

let r2 = f({ x: new Base(), y: new Derived2() }); // {}[]
AssertType(r2, "(union)[]");
AssertType(f({ x: new Base(), y: new Derived2() }), "(union)[]");
AssertType(f, "<T extends Base, U extends Base>({ x: T; y: U; }) => (union)[]");
AssertType({ x: new Base(), y: new Derived2() }, "{ x: Base; y: Derived2; }");
AssertType(x, "Base");
AssertType(new Base(), "Base");
AssertType(Base, "typeof Base");
AssertType(y, "Derived2");
AssertType(new Derived2(), "Derived2");
AssertType(Derived2, "typeof Derived2");


function f2<T extends Base, U extends Base>(a: { x: T; y: U }) {
AssertType((x: T) => a.y, "(T) => U");
AssertType(x, "T");
AssertType(a.y, "U");
    return (x: T) => a.y;
}

let r3 = f2({ x: new Derived(), y: new Derived2() }); // Derived => Derived2
AssertType(r3, "(Derived) => Derived2");
AssertType(f2({ x: new Derived(), y: new Derived2() }), "(Derived) => Derived2");
AssertType(f2, "<T extends Base, U extends Base>({ x: T; y: U; }) => (T) => U");
AssertType({ x: new Derived(), y: new Derived2() }, "{ x: Derived; y: Derived2; }");
AssertType(x, "Derived");
AssertType(new Derived(), "Derived");
AssertType(Derived, "typeof Derived");
AssertType(y, "Derived2");
AssertType(new Derived2(), "Derived2");
AssertType(Derived2, "typeof Derived2");

interface I<T, U> {
    x: T;
    y: U;
}

let i: I<Base, Derived>;
AssertType(i, "I<Base, Derived>");

let r4 = f2(i); // Base => Derived
AssertType(r4, "(Base) => Derived");
AssertType(f2(i), "(Base) => Derived");
AssertType(f2, "<T extends Base, U extends Base>({ x: T; y: U; }) => (T) => U");
AssertType(i, "I<Base, Derived>");


