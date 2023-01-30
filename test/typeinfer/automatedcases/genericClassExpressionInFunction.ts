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

// === tests/cases/conformance/classes/classExpressions/genericClassExpressionInFunction.ts ===
declare function AssertType(value:any, type:string):void;
class A<T> {
    genericVar: T
}
function B1<U>() {
    // class expression can use T
AssertType(class extends A<U> { }, "typeof (Anonymous class)");
    return class extends A<U> { 

AssertType(A, "A<U>");
}
}
class B2<V> {
    anon = class extends A<V> { }
}
function B3<W>() {
AssertType(class Inner<TInner> extends A<W> { }, "typeof Inner");
    return class Inner<TInner> extends A<W> { 

AssertType(Inner, "typeof Inner");

AssertType(A, "A<W>");
}
}
// extends can call B
class K extends B1<number>() {
    namae: string;
}
class C extends (new B2<number>().anon) {
    name: string;
}
let b3Number = B3<number>();
AssertType(b3Number, "typeof Inner");
AssertType(B3<number>(), "typeof Inner");
AssertType(B3, "<W>() => typeof Inner");

class S extends b3Number<string> {
    nom: string;
}
let c = new C();
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let k = new K();
AssertType(k, "K");
AssertType(new K(), "K");
AssertType(K, "typeof K");

let s = new S();
AssertType(s, "S");
AssertType(new S(), "S");
AssertType(S, "typeof S");

c.genericVar = 12;
AssertType(c.genericVar = 12, "int");
AssertType(c.genericVar, "number");
AssertType(12, "int");

k.genericVar = 12;
AssertType(k.genericVar = 12, "int");
AssertType(k.genericVar, "number");
AssertType(12, "int");

s.genericVar = 12;
AssertType(s.genericVar = 12, "int");
AssertType(s.genericVar, "number");
AssertType(12, "int");


