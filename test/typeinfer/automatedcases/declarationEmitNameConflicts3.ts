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

// === tests/cases/compiler/declarationEmitNameConflicts3.ts ===
declare function AssertType(value:any, type:string):void;

module M {
    export interface D { }
    export module D {
        export function f() { }
    }
    export module C {
        export function f() { }
    }
    export module E {
        export function f() { }
    }
}

module M.P {
    export class C {
        static f() { }
    }
    export class E extends C { }
    export enum D {
        f
    }
    export let v: M.D; // ok
    export let w = M.D.f; // error, should be typeof M.D.f
    export let x = M.C.f; // error, should be typeof M.C.f
    export let x = M.E.f; // error, should be typeof M.E.f
}

