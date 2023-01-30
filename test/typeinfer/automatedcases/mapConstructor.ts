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

// === tests/cases/compiler/mapConstructor.ts ===
declare function AssertType(value:any, type:string):void;
new Map();
AssertType(new Map(), "Map<any, any>");
AssertType(Map, "MapConstructor");

const potentiallyUndefinedIterable = [['1', 1], ['2', 2]] as Iterable<[string, number]> | undefined;
AssertType(potentiallyUndefinedIterable, "union");
AssertType([['1', 1], ['2', 2]] as Iterable<[string, number]> | undefined, "union");
AssertType([['1', 1], ['2', 2]], "[string, number][]");
AssertType(['1', 1], "[string, number]");
AssertType('1', "string");
AssertType(1, "int");
AssertType(['2', 2], "[string, number]");
AssertType('2', "string");
AssertType(2, "int");

new Map(potentiallyUndefinedIterable);
AssertType(new Map(potentiallyUndefinedIterable), "Map<string, number>");
AssertType(Map, "MapConstructor");
AssertType(potentiallyUndefinedIterable, "union");

const potentiallyNullIterable = [['1', 1], ['2', 2]] as Iterable<[string, number]> | null;
AssertType(potentiallyNullIterable, "union");
AssertType([['1', 1], ['2', 2]] as Iterable<[string, number]> | null, "union");
AssertType([['1', 1], ['2', 2]], "[string, number][]");
AssertType(['1', 1], "[string, number]");
AssertType('1', "string");
AssertType(1, "int");
AssertType(['2', 2], "[string, number]");
AssertType('2', "string");
AssertType(2, "int");
AssertType(null, "null");

new Map(potentiallyNullIterable);
AssertType(new Map(potentiallyNullIterable), "Map<string, number>");
AssertType(Map, "MapConstructor");
AssertType(potentiallyNullIterable, "union");


