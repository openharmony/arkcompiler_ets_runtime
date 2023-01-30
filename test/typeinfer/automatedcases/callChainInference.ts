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

// === tests/cases/conformance/expressions/optionalChaining/callChain/callChainInference.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #42404

interface Y {
    foo<T>(this: T, arg: keyof T): void;
    a: number;
    b: string;
}

declare const value: Y | undefined;
AssertType(value, "union");

if (value) {
    value?.foo("a");
AssertType(value?.foo("a"), "void");
AssertType(value?.foo, "<T>(T, keyof T) => void");
AssertType("a", "string");
}

value?.foo("a");
AssertType(value?.foo("a"), "union");
AssertType(value?.foo, "union");
AssertType("a", "string");


