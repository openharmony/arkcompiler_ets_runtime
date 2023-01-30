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

// === tests/cases/compiler/constraintOfRecursivelyMappedTypeWithConditionalIsResolvable.ts ===
declare function AssertType(value:any, type:string):void;

// https://github.com/Microsoft/TypeScript/issues/25379

interface Map<K, V> {
    // ...
}

export type ImmutableTypes = IImmutableMap<any>;

export type ImmutableModel<T> = { [K in keyof T]: T[K] extends ImmutableTypes ? T[K] : never };

export interface IImmutableMap<T extends ImmutableModel<T>> extends Map<string, any> {
    set<K extends keyof T>(key: K, value: T[K]): IImmutableMap<T>;
}

export type ImmutableTypes2 = IImmutableMap2<any>;
type isImmutableType<T> = [T] extends [ImmutableTypes2] ? T : never;
export type ImmutableModel2<T> = { [K in keyof T]: isImmutableType<T[K]> };
export interface IImmutableMap2<T extends ImmutableModel2<T>> extends Map<string, any> {
    set<K extends keyof T>(key: K, value: T[K]): IImmutableMap2<T>;
}


