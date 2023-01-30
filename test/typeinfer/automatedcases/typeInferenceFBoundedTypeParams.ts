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

// === tests/cases/compiler/typeInferenceFBoundedTypeParams.ts ===
declare function AssertType(value:any, type:string):void;
// Example from #6037

function fold<a, r>(values: a[], result: r, fold: (result: r, value: a) => r): r {
    for (let value of values) {
AssertType(value, "a");
AssertType(values, "a[]");

        result = fold(result, value);
AssertType(result = fold(result, value), "r");
AssertType(result, "r");
AssertType(fold(result, value), "r");
AssertType(fold, "(r, a) => r");
AssertType(result, "r");
AssertType(value, "a");
    }
AssertType(result, "r");
    return result;
}

function append<a, b extends a>(values: a[], value: b): a[] {
    values.push(value);
AssertType(values.push(value), "number");
AssertType(values.push, "(...a[]) => number");
AssertType(value, "b");

AssertType(values, "a[]");
    return values;
}

fold(
AssertType(fold(    [1, 2, 3],    [] as [string, string][],    (result, value) => append(        result,        ["", ""]    )), "[string, string][]");
AssertType(fold, "<a, r>(a[], r, (r, a) => r) => r");

    [1, 2, 3],
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

    [] as [string, string][],
AssertType([] as [string, string][], "[string, string][]");
AssertType([], "undefined[]");

    (result, value) => append(
AssertType((result, value) => append(        result,        ["", ""]    ), "([string, string][], number) => [string, string][]");
AssertType(result, "[string, string][]");
AssertType(value, "number");
AssertType(append(        result,        ["", ""]    ), "[string, string][]");
AssertType(append, "<a, b extends a>(a[], b) => a[]");

        result,
AssertType(result, "[string, string][]");

        ["", ""]
AssertType(["", ""], "[string, string]");
AssertType("", "string");
AssertType("", "string");

    )
);


