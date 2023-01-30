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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithObjectTypeArgsAndConstraints.ts ===
declare function AssertType(value:any, type:string):void;
// Generic call with constraints infering type parameter from object member properties
// No errors expected

class C {
    x: string;
}

class D {
    x: string;
    y: string;
}

class X<T> {
    x: T;
}

function foo<T extends { x: string }>(t: X<T>, t2: X<T>) {
    let x: T;
AssertType(x, "T");

AssertType(x, "T");
    return x;
}

let c1 = new X<C>();
AssertType(c1, "X<C>");
AssertType(new X<C>(), "X<C>");
AssertType(X, "typeof X");

let d1 = new X<D>();
AssertType(d1, "X<D>");
AssertType(new X<D>(), "X<D>");
AssertType(X, "typeof X");

let r = foo(c1, d1); 
AssertType(r, "C");
AssertType(foo(c1, d1), "C");
AssertType(foo, "<T extends { x: string; }>(X<T>, X<T>) => T");
AssertType(c1, "X<C>");
AssertType(d1, "X<D>");

let r2 = foo(c1, c1);
AssertType(r2, "C");
AssertType(foo(c1, c1), "C");
AssertType(foo, "<T extends { x: string; }>(X<T>, X<T>) => T");
AssertType(c1, "X<C>");
AssertType(c1, "X<C>");

function foo2<T extends C>(t: X<T>, t2: X<T>) {
    let x: T;
AssertType(x, "T");

AssertType(x, "T");
    return x;
}

let r = foo2(c1, d1); 
AssertType(r, "C");
AssertType(foo2(c1, d1), "C");
AssertType(foo2, "<T extends C>(X<T>, X<T>) => T");
AssertType(c1, "X<C>");
AssertType(d1, "X<D>");

let r2 = foo2(c1, c1);
AssertType(r2, "C");
AssertType(foo2(c1, c1), "C");
AssertType(foo2, "<T extends C>(X<T>, X<T>) => T");
AssertType(c1, "X<C>");
AssertType(c1, "X<C>");


