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

// === tests/cases/compiler/contextualTypeIterableUnions.ts ===
declare function AssertType(value:any, type:string):void;
declare class DMap<K, V>  {
  constructor(iterable: Iterable<[K, V]> | undefined);
}
new DMap([["1", 2]]);
AssertType(new DMap([["1", 2]]), "DMap<string, number>");
AssertType(DMap, "typeof DMap");
AssertType([["1", 2]], "[string, number][]");
AssertType(["1", 2], "[string, number]");
AssertType("1", "string");
AssertType(2, "int");

const i1: Iterable<{ a: true }> | undefined = [{ a: true }];
AssertType(i1, "union");
AssertType(a, "boolean");
AssertType(true, "boolean");
AssertType([{ a: true }], "{ a: true; }[]");
AssertType({ a: true }, "{ a: true; }");
AssertType(a, "boolean");
AssertType(true, "boolean");

const i2: Iterable<{ a: true }> | Iterable<{ b: false }> = [{ b: false }];
AssertType(i2, "union");
AssertType(a, "boolean");
AssertType(true, "boolean");
AssertType(b, "boolean");
AssertType(false, "boolean");
AssertType([{ b: false }], "{ b: false; }[]");
AssertType({ b: false }, "{ b: false; }");
AssertType(b, "boolean");
AssertType(false, "boolean");

const i3: Iterable<number> | 1[] = [2];
AssertType(i3, "union");
AssertType([2], "2[]");
AssertType(2, "int");


