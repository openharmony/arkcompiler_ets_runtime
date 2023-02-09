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

// === tests/cases/compiler/mapConstructorOnReadonlyTuple.ts ===
declare function AssertType(value:any, type:string):void;
const pairs = [['1', 1], ['2', 2]] as const
AssertType(pairs, "readonly [readonly ["1", 1], readonly ["2", 2]]");
AssertType([['1', 1], ['2', 2]] as const, "readonly [readonly ["1", 1], readonly ["2", 2]]");
AssertType([['1', 1], ['2', 2]], "readonly [readonly ["1", 1], readonly ["2", 2]]");
AssertType(['1', 1], "readonly ["1", 1]");
AssertType('1', "string");
AssertType(1, "int");
AssertType(['2', 2], "readonly ["2", 2]");
AssertType('2', "string");
AssertType(2, "int");

new Map(pairs);
AssertType(new Map(pairs), "Map<union, union>");
AssertType(Map, "MapConstructor");
AssertType(pairs, "readonly [readonly ["1", 1], readonly ["2", 2]]");


