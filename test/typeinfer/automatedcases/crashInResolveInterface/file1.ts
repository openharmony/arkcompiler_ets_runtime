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

// === tests/cases/compiler/file1.ts ===
declare function AssertType(value:any, type:string):void;
interface Q<T> {
    each(action: (item: T, index: number) => void): void;
}
let q1: Q<{ a: number; }>;
AssertType(q1, "Q<{ a: number; }>");
AssertType(a, "number");

let x = q1.each(x => c.log(x));
AssertType(x, "void");
AssertType(q1.each(x => c.log(x)), "void");
AssertType(q1.each, "(({ a: number; }, number) => void) => void");
AssertType(x => c.log(x), "({ a: number; }) => void");
AssertType(x, "{ a: number; }");
AssertType(c.log(x), "void");
AssertType(c.log, "(?any, ...any[]) => void");
AssertType(x, "{ a: number; }");


