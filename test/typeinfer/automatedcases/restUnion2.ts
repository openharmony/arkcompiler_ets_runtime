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

// === tests/cases/compiler/restUnion2.ts ===
declare function AssertType(value:any, type:string):void;
declare const undefinedUnion: { n: number } | undefined;
AssertType(undefinedUnion, "union");
AssertType(n, "number");

let rest2: { n: number };
AssertType(rest2, "{ n: number; }");
AssertType(n, "number");

let {...rest2 } = undefinedUnion;
AssertType(rest2, "{ n: number; }");
AssertType(undefinedUnion, "union");


declare const nullUnion: { n: number } | null;
AssertType(nullUnion, "union");
AssertType(n, "number");
AssertType(null, "null");

let rest3: { n: number };
AssertType(rest3, "{ n: number; }");
AssertType(n, "number");

let {...rest3 } = nullUnion;
AssertType(rest3, "{ n: number; }");
AssertType(nullUnion, "union");


