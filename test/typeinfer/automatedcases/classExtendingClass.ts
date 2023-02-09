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

// === tests/cases/conformance/classes/classDeclarations/classHeritageSpecification/classExtendingClass.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    foo: string;
    thing() { }
    static other() { }
}

class D extends C {
    bar: string;
}

let d: D;
AssertType(d, "D");

let r = d.foo;
AssertType(r, "string");
AssertType(d.foo, "string");

let r2 = d.bar;
AssertType(r2, "string");
AssertType(d.bar, "string");

let r3 = d.thing();
AssertType(r3, "void");
AssertType(d.thing(), "void");
AssertType(d.thing, "() => void");

let r4 = D.other();
AssertType(r4, "void");
AssertType(D.other(), "void");
AssertType(D.other, "() => void");

class C2<T> {
    foo: T;
    thing(x: T) { }
    static other<T>(x: T) { }
}

class D2<T> extends C2<T> {
    bar: string;
}

let d2: D2<string>;
AssertType(d2, "D2<string>");

let r5 = d2.foo;
AssertType(r5, "string");
AssertType(d2.foo, "string");

let r6 = d2.bar;
AssertType(r6, "string");
AssertType(d2.bar, "string");

let r7 = d2.thing('');
AssertType(r7, "void");
AssertType(d2.thing(''), "void");
AssertType(d2.thing, "(string) => void");
AssertType('', "string");

let r8 = D2.other(1);
AssertType(r8, "void");
AssertType(D2.other(1), "void");
AssertType(D2.other, "<T>(T) => void");
AssertType(1, "int");


