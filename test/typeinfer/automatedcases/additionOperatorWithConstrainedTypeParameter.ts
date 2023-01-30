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

// === tests/cases/conformance/expressions/binaryOperators/additionOperator/additionOperatorWithConstrainedTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;
// test for #17069
function sum<T extends Record<K, number>, K extends string>(n: number, v: T, k: K) {
    n = n + v[k];
AssertType(n = n + v[k], "number");
AssertType(n, "number");
AssertType(n + v[k], "number");
AssertType(n, "number");
AssertType(v[k], "T[K]");
AssertType(v, "T");
AssertType(k, "K");

    n += v[k]; // += should work the same way
AssertType(n += v[k], "number");
AssertType(n, "number");
AssertType(v[k], "T[K]");
AssertType(v, "T");
AssertType(k, "K");
}
function realSum<T extends Record<K, number>, K extends string>(n: number, vs: T[], k: K) {
    for (const v of vs) {
AssertType(v, "T");
AssertType(vs, "T[]");

        n = n + v[k];
AssertType(n = n + v[k], "number");
AssertType(n, "number");
AssertType(n + v[k], "number");
AssertType(n, "number");
AssertType(v[k], "T[K]");
AssertType(v, "T");
AssertType(k, "K");

        n += v[k];
AssertType(n += v[k], "number");
AssertType(n, "number");
AssertType(v[k], "T[K]");
AssertType(v, "T");
AssertType(k, "K");
    }
}


