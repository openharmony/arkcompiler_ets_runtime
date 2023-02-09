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

// === tests/cases/compiler/overloadBindingAcrossDeclarationBoundaries.ts ===
declare function AssertType(value:any, type:string):void;
interface Opt1 {
    p?: any;
}
interface Opt2 {
    q?: any;
}
interface Opt3 {
    r?: any;
}
interface Opt4 {
    s?: any;
}
interface A {
    a(o: Opt1): Opt1;
    a(o: Opt2): Opt2;
    (o: Opt1): Opt1;
    (o: Opt2): Opt2;
    new (o: Opt1): Opt1;
    new (o: Opt2): Opt2;
}
interface A {
    a(o: Opt3): Opt3;
    a(o: Opt4): Opt4;
    (o: Opt3): Opt3;
    (o: Opt4): Opt4;
    new (o: Opt3): Opt3;
    new (o: Opt4): Opt4;
}

let a: A;
AssertType(a, "A");

// These should all be Opt3
let a1 = a.a({});
AssertType(a1, "Opt3");
AssertType(a.a({}), "Opt3");
AssertType(a.a, "{ (Opt1): Opt1; (Opt2): Opt2; (Opt3): Opt3; (Opt4): Opt4; }");
AssertType({}, "{}");

let a1 = a({});
AssertType(a1, "Opt3");
AssertType(a({}), "Opt3");
AssertType(a, "A");
AssertType({}, "{}");

let a1 = new a({});
AssertType(a1, "Opt3");
AssertType(new a({}), "Opt3");
AssertType(a, "A");
AssertType({}, "{}");


