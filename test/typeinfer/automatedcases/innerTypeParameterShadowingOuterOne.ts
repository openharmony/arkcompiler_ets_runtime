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

// === tests/cases/conformance/types/typeParameters/typeParameterLists/innerTypeParameterShadowingOuterOne.ts ===
declare function AssertType(value:any, type:string):void;
// inner type parameters shadow outer ones of the same name
// no errors expected

function f<T extends Date>() {
    function g<T extends Number>() {
AssertType(g, "<T extends Number>() => void");

        let x: T;
AssertType(x, "T");

        x.toFixed();
AssertType(x.toFixed(), "string");
AssertType(x.toFixed, "(?number) => string");
    }
    let x: T;
AssertType(x, "T");

    x.getDate();
AssertType(x.getDate(), "number");
AssertType(x.getDate, "() => number");
}

function f2<T extends Date, U extends Date>() {
    function g<T extends Number, U extends Number>() {
AssertType(g, "<T extends Number, U extends Number>() => void");

        let x: U;
AssertType(x, "U");

        x.toFixed();
AssertType(x.toFixed(), "string");
AssertType(x.toFixed, "(?number) => string");
    }
    let x: U;
AssertType(x, "U");

    x.getDate();
AssertType(x.getDate(), "number");
AssertType(x.getDate, "() => number");
}

