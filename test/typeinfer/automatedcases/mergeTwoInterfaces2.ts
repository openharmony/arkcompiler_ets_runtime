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

// === tests/cases/conformance/interfaces/declarationMerging/mergeTwoInterfaces2.ts ===
declare function AssertType(value:any, type:string):void;

// two interfaces with the same root module should merge

// root module now multiple module declarations
module M2 {
    export interface A {
        foo: string;
    }

    let a: A;
    let r1 = a.foo
    let r2 = a.bar;
}

module M2 {
    export interface A {
        bar: number;
    }

    let a: A;
    let r1 = a.foo
    let r2 = a.bar;
}

// same as above but with an additional level of nesting
module M2 {
    export module M3 {
        export interface A {
            foo: string;
        }

        let a: A;
        let r1 = a.foo
        let r2 = a.bar;
    }
}

module M2 {
    export module M3 {
        export interface A {
            bar: number;
        }

        let a: A;
        let r1 = a.foo
        let r2 = a.bar;
    }
}

