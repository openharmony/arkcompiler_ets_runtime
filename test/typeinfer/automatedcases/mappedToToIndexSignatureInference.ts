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

// === tests/cases/compiler/mappedToToIndexSignatureInference.ts ===
declare function AssertType(value:any, type:string):void;
declare const fn: <K extends string, V>(object: { [Key in K]: V }) => object;
AssertType(fn, "<K extends string, V>({ [Key in K]: V; }) => object");
AssertType(object, "{ [Key in K]: V; }");

declare const a: { [index: string]: number };
AssertType(a, "{ [string]: number; }");
AssertType(index, "string");

fn(a);
AssertType(fn(a), "object");
AssertType(fn, "<K extends string, V>({ [Key in K]: V; }) => object");
AssertType(a, "{ [string]: number; }");

// Repro from #30218

declare function enumValues<K extends string, V extends string>(e: Record<K, V>): V[];

enum E { A = 'foo', B = 'bar' }

let x: E[] = enumValues(E);
AssertType(x, "E[]");
AssertType(enumValues(E), "E[]");
AssertType(enumValues, "<K extends string, V extends string>(Record<K, V>) => V[]");
AssertType(E, "typeof E");


