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

// === tests/cases/compiler/cyclicGenericTypeInstantiation.ts ===
declare function AssertType(value:any, type:string):void;
function foo<T>() {
    let z = foo<typeof y>();
AssertType(z, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(foo<typeof y>(), "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(foo, "<T>() => { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(y, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");

    let y: {
AssertType(y, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");

        y2: typeof z
AssertType(y2, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(z, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");

    };
AssertType(y, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
    return y;
}


function bar<T>() {
    let z = bar<typeof y>();
AssertType(z, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(bar<typeof y>(), "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(bar, "<T>() => { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(y, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");

    let y: {
AssertType(y, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");

        y2: typeof z;
AssertType(y2, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(z, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
    }
AssertType(y, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
    return y;
}

let a = foo<number>();
AssertType(a, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(foo<number>(), "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(foo, "<T>() => { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");

let b = bar<number>();
AssertType(b, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(bar<number>(), "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(bar, "<T>() => { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");

a = b;
AssertType(a = b, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(a, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");
AssertType(b, "{ y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: { y2: any; }; }; }; }; }; }; }; }; }; }; }");


