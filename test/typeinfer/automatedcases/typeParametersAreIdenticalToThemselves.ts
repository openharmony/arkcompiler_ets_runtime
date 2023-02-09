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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/typeParametersAreIdenticalToThemselves.ts ===
declare function AssertType(value:any, type:string):void;
// type parameters from the same declaration are identical to themself

function foo1<T>(x: T);
function foo1<T>(x: T); // no error, different declaration for each T
function foo1<T>(x: T) { }

function foo2<T, U>(x: T);
function foo2<T, U>(x: T); // no error, different declaration for each T
function foo2<T, U>(x: T) { }

function foo3<T, U>(x: T, y: U) {
    function inner(x: T);
AssertType(inner, "{ (T): any; (T): any; }");
AssertType(x, "T");

    function inner(x: T); // error, same T
AssertType(inner, "{ (T): any; (T): any; }");
AssertType(x, "T");

    function inner(x: T) { 
AssertType(inner, "{ (T): any; (T): any; }");

AssertType(x, "T");
}

    function inner2(x: T);
AssertType(inner2, "{ (T): any; <T>(T): any; }");
AssertType(x, "T");

    function inner2<T>(x: T); // no error, different T
AssertType(inner2, "{ (T): any; <T>(T): any; }");
AssertType(x, "T");

    function inner2(x: any) { 
AssertType(inner2, "{ (T): any; <T>(T): any; }");

AssertType(x, "any");
}
}

class C<T> {
    foo1(x: T);
    foo1(x: T); // error, same T
    foo1(x: T) { }

    foo2<U>(a: T, x: U);
    foo2<U>(a: T, x: U); // no error, different declaration for each U
    foo2<U>(a: T, x: U) { }

    foo3<T>(x: T);
    foo3<T>(x: T); // no error, different declaration for each T
    foo3<T>(x: T) { }

    foo4<T extends Date>(x: T);
    foo4<T extends Date>(x: T); // no error, different declaration for each T
    foo4<T extends Date>(x: T) { }
}

class C2<T extends Date> {
    foo1(x: T);
    foo1(x: T); // error, same T
    foo1(x: T) { }

    foo2<U>(a: T, x: U);
    foo2<U>(a: T, x: U); // no error, different declaration for each U
    foo2<U>(a: T, x: U) { }

    foo3<T>(x: T);
    foo3<T>(x: T); // no error, different declaration for each T
    foo3<T>(x: T) { }
}

interface I<T> {
    foo1(x: T);
    foo1(x: T); // error, same T

    foo2<U>(a: T, x: U);
    foo2<U>(a: T, x: U); // no error, different declaration for each U

    foo3<T>(x: T);
    foo3<T>(x: T); // no error, different declaration for each T

    foo4<T extends Date>(x: T);
    foo4<T extends Date>(x: T); // no error, different declaration for each T
}

interface I2<T extends Date> {
    foo1(x: T);
    foo1(x: T); // error, same T

    foo2<U>(a: T, x: U);
    foo2<U>(a: T, x: U); // no error, different declaration for each U

    foo3<T>(x: T);
    foo3<T>(x: T); // no error, different declaration for each T
}

