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

// === tests/cases/compiler/declFileTypeAnnotationVisibilityErrorVariableDeclaration.ts ===
declare function AssertType(value:any, type:string):void;

module m {
    class private1 {
    }

    export class public1 {
    }

    // Directly using names from this module
    let x: private1;
    let y = new private1();

    export let k: private1;
    export let l = new private1();

    let x2: public1;
    let y2 = new public1();

    export let k2: public1;
    export let l2 = new public1();

    module m2 {
        export class public2 {
        }
    }

    let x3: m2.public2;
    let y3 = new m2.public2();

    export let k3: m2.public2;
    export let l3 = new m2.public2();
}


