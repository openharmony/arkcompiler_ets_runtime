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

// === tests/cases/compiler/declFileTypeAnnotationVisibilityErrorAccessors.ts ===
declare function AssertType(value:any, type:string):void;
module m {
    class private1 {
    }

    export class public1 {
    }

    module m2 {
        export class public2 {
        }
    }

    export class c {
        // getter with annotation
        get foo1(): private1 {
            return;
        }

        // getter without annotation
        get foo2() {
AssertType(new private1(), "private1");
AssertType(private1, "typeof private1");
            return new private1();
        }

        // setter with annotation
        set foo3(param: private1) {
        }

        // Both - getter without annotation, setter with annotation
        get foo4() {
AssertType(new private1(), "private1");
AssertType(private1, "typeof private1");
            return new private1();
        }
        set foo4(param: private1) {
        }

        // Both - with annotation
        get foo5(): private1 {
            return;
        }
        set foo5(param: private1) {
        }

        // getter with annotation
        get foo11(): public1 {
            return;
        }

        // getter without annotation
        get foo12() {
AssertType(new public1(), "public1");
AssertType(public1, "typeof public1");
            return new public1();
        }

        // setter with annotation
        set foo13(param: public1) {
        }

        // Both - getter without annotation, setter with annotation
        get foo14() {
AssertType(new public1(), "public1");
AssertType(public1, "typeof public1");
            return new public1();
        }
        set foo14(param: public1) {
        }

        // Both - with annotation
        get foo15(): public1 {
            return;
        }
        set foo15(param: public1) {
        }

        // getter with annotation
        get foo111(): m2.public2 {
            return;
        }

        // getter without annotation
        get foo112() {
AssertType(new m2.public2(), "m2.public2");
AssertType(m2.public2, "typeof m2.public2");
            return new m2.public2();
        }

        // setter with annotation
        set foo113(param: m2.public2) {
        }

        // Both - getter without annotation, setter with annotation
        get foo114() {
AssertType(new m2.public2(), "m2.public2");
AssertType(m2.public2, "typeof m2.public2");
            return new m2.public2();
        }
        set foo114(param: m2.public2) {
        }

        // Both - with annotation
        get foo115(): m2.public2 {
            return;
        }
        set foo115(param: m2.public2) {
        }
    }
}


