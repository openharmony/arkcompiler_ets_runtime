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

// === tests/cases/compiler/cyclicTypeInstantiation.ts ===
declare function AssertType(value:any, type:string):void;
function foo<T>() {
    let x: {
AssertType(x, "{ a: T; b: any; }");

        a: T;
AssertType(a, "T");

        b: typeof x;
AssertType(b, "{ a: T; b: any; }");
AssertType(x, "{ a: T; b: any; }");

    };
AssertType(x, "{ a: T; b: any; }");
    return x;
}

function bar<T>() {
    let x: {
AssertType(x, "{ a: T; b: any; }");

        a: T;
AssertType(a, "T");

        b: typeof x;
AssertType(b, "{ a: T; b: any; }");
AssertType(x, "{ a: T; b: any; }");

    };
AssertType(x, "{ a: T; b: any; }");
    return x;
}

let a = foo<string>();
AssertType(a, "{ a: string; b: any; }");
AssertType(foo<string>(), "{ a: string; b: any; }");
AssertType(foo, "<T>() => { a: T; b: any; }");

let b = bar<string>();
AssertType(b, "{ a: string; b: any; }");
AssertType(bar<string>(), "{ a: string; b: any; }");
AssertType(bar, "<T>() => { a: T; b: any; }");

// Relating types of a and b produces instantiations of the cyclic anonymous types in foo and bar
a = b;
AssertType(a = b, "{ a: string; b: any; }");
AssertType(a, "{ a: string; b: any; }");
AssertType(b, "{ a: string; b: any; }");


