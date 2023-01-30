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

// === tests/cases/conformance/expressions/nullishCoalescingOperator/nullishCoalescingOperator3.ts ===
declare function AssertType(value:any, type:string):void;
declare const a1: 'literal' | undefined | null
AssertType(a1, "union");
AssertType(null, "null");

declare const a2: '' | undefined | null
AssertType(a2, "union");
AssertType(null, "null");

declare const a3: 1 | undefined | null
AssertType(a3, "union");
AssertType(null, "null");

declare const a4: 0 | undefined | null
AssertType(a4, "union");
AssertType(null, "null");

declare const a5: true | undefined | null
AssertType(a5, "union");
AssertType(true, "boolean");
AssertType(null, "null");

declare const a6: false | undefined | null
AssertType(a6, "union");
AssertType(false, "boolean");
AssertType(null, "null");


const aa1 = a1 ?? a2 ?? a3 ?? a4 ?? a5 ?? a6 ?? 'whatever'
AssertType(aa1, "union");
AssertType(a1 ?? a2 ?? a3 ?? a4 ?? a5 ?? a6 ?? 'whatever', "union");
AssertType(a1 ?? a2 ?? a3 ?? a4 ?? a5 ?? a6, "union");
AssertType(a1 ?? a2 ?? a3 ?? a4 ?? a5, "union");
AssertType(a1 ?? a2 ?? a3 ?? a4, "union");
AssertType(a1 ?? a2 ?? a3, "union");
AssertType(a1 ?? a2, "union");
AssertType(a1, "union");
AssertType(a2, "union");
AssertType(a3, "union");
AssertType(a4, "union");
AssertType(a5, "union");
AssertType(a6, "union");
AssertType('whatever', "string");


