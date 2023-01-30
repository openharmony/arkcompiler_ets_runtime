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

// === tests/cases/compiler/declFileTypeAnnotationVisibilityErrorTypeLiteral.ts ===
declare function AssertType(value:any, type:string):void;
module m {
    class private1 {
    }
    module m2 {
        export class public1 {
        }
    }

    export let x: {
        x: private1;
        y: m2.public1;
        (): m2.public1[];
        method(): private1;
        [n: number]: private1;
        [s: string]: m2.public1;
    };
    export let x2 = {
        x: new private1(),
        y: new m2.public1(),
        method() {
AssertType(new private1(), "private1");
AssertType(private1, "typeof private1");
            return new private1();
        }
    };
    export let x3 = x;

    // Function type
    export let y: (a: private1) => m2.public1;
    export let y2 = y;

    // constructor type
    export let z: new (a: private1) => m2.public1;
    export let z2 = z;
}

