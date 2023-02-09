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

// === tests/cases/compiler/nonNullMappedType.ts ===
declare function AssertType(value:any, type:string):void;
function f<A extends string>(p0: { [key in A]: {} | undefined }, p1: A) {
    const v: {} = p0[p1]!;
AssertType(v, "{}");
AssertType(p0[p1]!, "{}");
AssertType(p0[p1], "union");
AssertType(p0, "{ [key in A]: union; }");
AssertType(p1, "A");
}

