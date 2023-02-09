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

// === tests/cases/compiler/declarationEmit_nameConflicts_0.ts ===
declare function AssertType(value:any, type:string):void;

import * as im from './declarationEmit_nameConflicts_1';
export module M {
    export function f() { }
    export class C { }
    export module N {
        export function g() { };
        export interface I { }
    }

    export import a = M.f;
    export import b = M.C;
    export import c = N;
    export import d = im;
}

export module M.P {
    export function f() { }
    export class C { }
    export module N {
        export function g() { };
        export interface I { }
    }
    export import im = M.P.f;
    export let a = M.a; // emitted incorrectly as typeof f
    export let b = M.b; // ok
    export let c = M.c; // ok
    export let g = M.c.g; // ok
    export let d = M.d; // emitted incorrectly as typeof im
}

export module M.Q {
    export function f() { }
    export class C { }
    export module N {
        export function g() { };
        export interface I { }
    }
    export interface b extends M.b { } // ok
    export interface I extends M.c.I { } // ok
    export module c {
        export interface I extends M.c.I { } // ok
    }
}

