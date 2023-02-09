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

// === tests/cases/compiler/constEnumToStringWithComments.ts ===
declare function AssertType(value:any, type:string):void;
const enum Foo {
    X = 100,
    Y = 0.5,
    Z = 2.,
    A = -1,
    B = -1.5,
    C = -1.
}

let x0 = Foo.X.toString();
AssertType(x0, "string");
AssertType(Foo.X.toString(), "string");
AssertType(Foo.X.toString, "(?number) => string");

let x1 = Foo["X"].toString();
AssertType(x1, "string");
AssertType(Foo["X"].toString(), "string");
AssertType(Foo["X"].toString, "(?number) => string");

let y0 = Foo.Y.toString();
AssertType(y0, "string");
AssertType(Foo.Y.toString(), "string");
AssertType(Foo.Y.toString, "(?number) => string");

let y1 = Foo["Y"].toString();
AssertType(y1, "string");
AssertType(Foo["Y"].toString(), "string");
AssertType(Foo["Y"].toString, "(?number) => string");

let z0 = Foo.Z.toString();
AssertType(z0, "string");
AssertType(Foo.Z.toString(), "string");
AssertType(Foo.Z.toString, "(?number) => string");

let z1 = Foo["Z"].toString();
AssertType(z1, "string");
AssertType(Foo["Z"].toString(), "string");
AssertType(Foo["Z"].toString, "(?number) => string");

let a0 = Foo.A.toString();
AssertType(a0, "string");
AssertType(Foo.A.toString(), "string");
AssertType(Foo.A.toString, "(?number) => string");

let a1 = Foo["A"].toString();
AssertType(a1, "string");
AssertType(Foo["A"].toString(), "string");
AssertType(Foo["A"].toString, "(?number) => string");

let b0 = Foo.B.toString();
AssertType(b0, "string");
AssertType(Foo.B.toString(), "string");
AssertType(Foo.B.toString, "(?number) => string");

let b1 = Foo["B"].toString();
AssertType(b1, "string");
AssertType(Foo["B"].toString(), "string");
AssertType(Foo["B"].toString, "(?number) => string");

let c0 = Foo.C.toString();
AssertType(c0, "string");
AssertType(Foo.C.toString(), "string");
AssertType(Foo.C.toString, "(?number) => string");

let c1 = Foo["C"].toString();
AssertType(c1, "string");
AssertType(Foo["C"].toString(), "string");
AssertType(Foo["C"].toString, "(?number) => string");


