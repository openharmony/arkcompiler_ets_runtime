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

// === tests/cases/compiler/declFileTypeAnnotationVisibilityErrorParameterOfFunction.ts ===
declare function AssertType(value:any, type:string):void;

module m {
    class private1 {
    }

    export class public1 {
    }

    // Directly using names from this module
    function foo1(param: private1) {
    }
    function foo2(param = new private1()) {
    }

    export function foo3(param : private1) {
    }
    export function foo4(param = new private1()) {
    }

    function foo11(param: public1) {
    }
    function foo12(param = new public1()) {
    }

    export function foo13(param: public1) {
    }
    export function foo14(param = new public1()) {
    }

    module m2 {
        export class public2 {
        }
    }

    function foo111(param: m2.public2) {
    }
    function foo112(param = new m2.public2()) {
    }

    export function foo113(param: m2.public2) {
    }
    export function foo114(param = new m2.public2()) {
    }
}


