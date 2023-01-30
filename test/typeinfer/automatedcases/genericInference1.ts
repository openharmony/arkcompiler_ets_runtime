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

// === tests/cases/compiler/genericInference1.ts ===
declare function AssertType(value:any, type:string):void;
['a', 'b', 'c'].map(x => x.length);
AssertType(['a', 'b', 'c'].map(x => x.length), "number[]");
AssertType(['a', 'b', 'c'].map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(x => x.length, "(string) => number");
AssertType(x, "string");
AssertType(x.length, "number");


