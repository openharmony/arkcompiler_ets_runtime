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

// === tests/cases/compiler/module.ts ===
declare function AssertType(value:any, type:string):void;
export const c0 = "a";
AssertType(c0, "string");
AssertType("a", "string");

export const c1 = 1;
AssertType(c1, "int");
AssertType(1, "int");

export const s0 = Symbol();
AssertType(s0, "unique symbol");
AssertType(Symbol(), "unique symbol");
AssertType(Symbol, "SymbolConstructor");

export interface T0 {
    [c0]: number;
    [c1]: string;
    [s0]: boolean;
}
export declare class T1 implements T2 {
    [c0]: number;
    [c1]: string;
    [s0]: boolean;
}
export declare class T2 extends T1 {
}
export declare type T3 = {
    [c0]: number;
    [c1]: string;
    [s0]: boolean;
};


