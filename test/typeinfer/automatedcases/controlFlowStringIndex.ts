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

// === tests/cases/conformance/controlFlow/controlFlowStringIndex.ts ===
declare function AssertType(value:any, type:string):void;
type A = {
    other: number | null;
    [index: string]: number | null
};
declare const value: A;
AssertType(value, "A");

if (value.foo !== null) {
    value.foo.toExponential()
AssertType(value.foo.toExponential(), "string");
AssertType(value.foo.toExponential, "(?union) => string");
AssertType(value.foo, "number");

    value.other // should still be number | null
AssertType(value.other, "union");

    value.bar // should still be number | null
AssertType(value.bar, "union");
}


