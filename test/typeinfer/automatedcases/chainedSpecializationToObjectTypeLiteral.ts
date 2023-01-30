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

// === tests/cases/compiler/chainedSpecializationToObjectTypeLiteral.ts ===
declare function AssertType(value:any, type:string):void;
interface Sequence<T> {
    each(iterator: (value: T) => void): void;
    map<U>(iterator: (value: T) => U): Sequence<U>;
    filter(iterator: (value: T) => boolean): Sequence<T>;
    groupBy<K>(keySelector: (value: T) => K): Sequence<{ key: K; items: T[]; }>;
}

let s: Sequence<string>;
AssertType(s, "Sequence<string>");

let s2 = s.groupBy(s => s.length);
AssertType(s2, "Sequence<{ key: number; items: string[]; }>");
AssertType(s.groupBy(s => s.length), "Sequence<{ key: number; items: string[]; }>");
AssertType(s.groupBy, "<K>((string) => K) => Sequence<{ key: K; items: string[]; }>");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");

let s3 = s2.each(x => { x.key /* Type is K, should be number */ });
AssertType(s3, "void");
AssertType(s2.each(x => { x.key /* Type is K, should be number */ }), "void");
AssertType(s2.each, "(({ key: number; items: string[]; }) => void) => void");
AssertType(x => { x.key /* Type is K, should be number */ }, "({ key: number; items: string[]; }) => void");
AssertType(x, "{ key: number; items: string[]; }");
AssertType(x.key, "number");


