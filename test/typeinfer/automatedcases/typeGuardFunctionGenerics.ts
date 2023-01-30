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

// === tests/cases/conformance/expressions/typeGuards/typeGuardFunctionGenerics.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    propA: number;
}

class B {
    propB: number;
}

class C extends A {
    propC: number;
}

declare function isB(p1): p1 is B;
declare function isC(p1): p1 is C;
declare function retC(x): C; 

declare function funA<T>(p1: (p1) => T): T;
declare function funB<T>(p1: (p1) => T, p2: any): p2 is T;
declare function funC<T>(p1: (p1) => p1 is T): T;
declare function funD<T>(p1: (p1) => p1 is T, p2: any): p2 is T;
declare function funE<T, U>(p1: (p1) => p1 is T, p2: U): T;

let a: A;
AssertType(a, "A");

let test1: boolean = funA(isB);
AssertType(test1, "boolean");
AssertType(funA(isB), "boolean");
AssertType(funA, "<T>((any) => T) => T");
AssertType(isB, "(any) => p1 is B");

if (funB(retC, a)) {
    a.propC;
AssertType(a.propC, "number");
}
let test2: B = funC(isB);
AssertType(test2, "B");
AssertType(funC(isB), "B");
AssertType(funC, "<T>((any) => p1 is T) => T");
AssertType(isB, "(any) => p1 is B");

if (funD(isC, a)) {
    a.propC;
AssertType(a.propC, "number");
}
let test3: B = funE(isB, 1);
AssertType(test3, "B");
AssertType(funE(isB, 1), "B");
AssertType(funE, "<T, U>((any) => p1 is T, U) => T");
AssertType(isB, "(any) => p1 is B");
AssertType(1, "int");


