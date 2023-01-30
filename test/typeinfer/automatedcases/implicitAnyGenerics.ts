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

// === tests/cases/compiler/implicitAnyGenerics.ts ===
declare function AssertType(value:any, type:string):void;
class C<T> {
    x: T;
}

let c = new C();
AssertType(c, "C<unknown>");
AssertType(new C(), "C<unknown>");
AssertType(C, "typeof C");

let c2 = new C<any>();
AssertType(c2, "C<any>");
AssertType(new C<any>(), "C<any>");
AssertType(C, "typeof C");

let c3 = new C<number>();
AssertType(c3, "C<number>");
AssertType(new C<number>(), "C<number>");
AssertType(C, "typeof C");

let c4: C<any> = new C();
AssertType(c4, "C<any>");
AssertType(new C(), "C<any>");
AssertType(C, "typeof C");

class D<T> {
    constructor(x: T) { }
}

let d = new D(null);
AssertType(d, "D<any>");
AssertType(new D(null), "D<any>");
AssertType(D, "typeof D");
AssertType(null, "null");

let d2 = new D(1);
AssertType(d2, "D<number>");
AssertType(new D(1), "D<number>");
AssertType(D, "typeof D");
AssertType(1, "int");

let d3 = new D<any>(1);
AssertType(d3, "D<any>");
AssertType(new D<any>(1), "D<any>");
AssertType(D, "typeof D");
AssertType(1, "int");

let d4 = new D(<any>1);
AssertType(d4, "D<any>");
AssertType(new D(<any>1), "D<any>");
AssertType(D, "typeof D");
AssertType(<any>1, "any");
AssertType(1, "int");

let d5: D<any> = new D(null);
AssertType(d5, "D<any>");
AssertType(new D(null), "D<any>");
AssertType(D, "typeof D");
AssertType(null, "null");

function foo<T>(): T { 
AssertType(null, "null");
return null; };

foo() 
AssertType(foo(), "unknown");
AssertType(foo, "<T>() => T");

foo<any>();
AssertType(foo<any>(), "any");
AssertType(foo, "<T>() => T");


    

