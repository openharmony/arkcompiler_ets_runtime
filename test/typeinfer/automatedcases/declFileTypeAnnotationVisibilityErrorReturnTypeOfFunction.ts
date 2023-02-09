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

// === tests/cases/compiler/declFileTypeAnnotationVisibilityErrorReturnTypeOfFunction.ts ===
declare function AssertType(value:any, type:string):void;
module m {
    class private1 {
    }

    export class public1 {
    }

    // Directly using names from this module
    function foo1(): private1 {
        return;
    }
    function foo2() {
AssertType(new private1(), "private1");
AssertType(private1, "typeof private1");
        return new private1();
    }

    export function foo3(): private1 {
        return;
    }
    export function foo4() {
AssertType(new private1(), "private1");
AssertType(private1, "typeof private1");
        return new private1();
    }

    function foo11(): public1 {
        return;
    }
    function foo12() {
AssertType(new public1(), "public1");
AssertType(public1, "typeof public1");
        return new public1();
    }

    export function foo13(): public1 {
        return;
    }
    export function foo14() {
AssertType(new public1(), "public1");
AssertType(public1, "typeof public1");
        return new public1();
    }

    module m2 {
        export class public2 {
        }
    }

    function foo111(): m2.public2 {
        return;
    }
    function foo112() {
AssertType(new m2.public2(), "m2.public2");
AssertType(m2.public2, "typeof m2.public2");
        return new m2.public2();
    }

    export function foo113(): m2.public2 {
        return;
    }
    export function foo114() {
AssertType(new m2.public2(), "m2.public2");
AssertType(m2.public2, "typeof m2.public2");
        return new m2.public2();
    }
}


