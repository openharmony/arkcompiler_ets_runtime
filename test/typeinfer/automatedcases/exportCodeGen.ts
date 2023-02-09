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

// === tests/cases/conformance/internalModules/codeGeneration/exportCodeGen.ts ===
declare function AssertType(value:any, type:string):void;
// should replace all refs to 'x' in the body,
// with fully qualified
module A {
    export let x = 12;
    function lt12() {
AssertType(x < 12, "boolean");
AssertType(x, "number");
AssertType(12, "int");
        return x < 12;
    }
} 

// should not fully qualify 'x'
module B {
    let x = 12;
    function lt12() {
AssertType(x < 12, "boolean");
AssertType(x, "number");
AssertType(12, "int");
        return x < 12;
    }
}

// not copied, since not exported
module C {
    function no() {
AssertType(false, "boolean");
        return false;
    }
}

// copies, since exported
module D {
    export function yes() {
AssertType(true, "boolean");
        return true;
    }
}

// validate all exportable statements
module E {
    export enum Color { Red }
    export function fn() { }
    export interface I { id: number }
    export class C { name: string }
    export module M {
        export let x = 42;
    }
}

// validate all exportable statements,
// which are not exported
module F {
    enum Color { Red }
    function fn() { }
    interface I { id: number }
    class C { name: string }
    module M {
        let x = 42;
    }
}

