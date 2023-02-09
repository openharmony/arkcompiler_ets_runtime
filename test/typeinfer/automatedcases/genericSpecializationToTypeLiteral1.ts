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

// === tests/cases/compiler/genericSpecializationToTypeLiteral1.ts ===
declare function AssertType(value:any, type:string):void;

interface IEnumerable<T> {

    zip<TResult>(second: IEnumerable<T>, resultSelector: (first: T, second: T, index: number) => TResult): IEnumerable<TResult>;
    zip<TResult>(second: T[], resultSelector: (first: T, second: T, index: number) => TResult): IEnumerable<TResult>;
    zip<TResult>(...params: any[]): IEnumerable<TResult>; // last one is selector

    merge<TResult>(...params: IEnumerable<T>[]): IEnumerable<T>;
    merge<TResult>(...params: T[][]): IEnumerable<T>;


    concat(...sequences: IEnumerable<T>[]): IEnumerable<T>;
    concat(...sequences: T[]): IEnumerable<T>;

    insert(index: number, second: IEnumerable<T>): IEnumerable<T>;

    sequenceEqual(second: IEnumerable<T>): boolean;
    sequenceEqual<TCompare>(second: IEnumerable<T>, compareSelector: (element: T) => TCompare): boolean;
    sequenceEqual(second: T[]): boolean;
    sequenceEqual<TCompare>(second: T[], compareSelector: (element: T) => TCompare): boolean;   
    
    toDictionary<TKey>(keySelector: (element: T) => TKey): IDictionary<TKey, any>;
    toDictionary<TKey, TValue>(keySelector: (element: T) => TKey, elementSelector: (element: T) => TValue): IDictionary<TKey, TValue>;
    toDictionary<TKey, TValue, TCompare>(keySelector: (element: T) => TKey, elementSelector: (element: T) => TValue, compareSelector: (key: TKey) => TCompare): IDictionary<TKey, TValue>;
}

interface IDictionary<TKey, TValue> {
    toEnumerable(): IEnumerable<{ key: TKey; value: TValue }>;
}

