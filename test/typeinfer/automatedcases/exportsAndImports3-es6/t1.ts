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

// === tests/cases/conformance/es6/modules/t1.ts ===
declare function AssertType(value:any, type:string):void;
export let v = 1;
AssertType(v, "number");
AssertType(1, "int");

export function f() { }
export class C {
}
export interface I {
}
export enum E {
    A, B, C
}
export const enum D {
    A, B, C
}
export module M {
    export let x;
}
export module N {
    export interface I {
    }
}
export type T = number;
export import a = M.x;

export { v as v1, f as f1, C as C1, I as I1, E as E1, D as D1, M as M1, N as N1, T as T1, a as a1 };


