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

// === tests/cases/compiler/typePredicateStructuralMatch.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #12235

getResults1([]);
AssertType(getResults1([]), "Results");
AssertType(getResults1, "(union) => Results");
AssertType([], "undefined[]");

getResults1({data: []});
AssertType(getResults1({data: []}), "Results");
AssertType(getResults1, "(union) => Results");
AssertType({data: []}, "{ data: undefined[]; }");
AssertType(data, "undefined[]");
AssertType([], "undefined[]");

getResults2([]);
AssertType(getResults2([]), "Results");
AssertType(getResults2, "(union) => Results");
AssertType([], "undefined[]");

getResults2({data: []});
AssertType(getResults2({data: []}), "Results");
AssertType(getResults2, "(union) => Results");
AssertType({data: []}, "{ data: undefined[]; }");
AssertType(data, "undefined[]");
AssertType([], "undefined[]");

type Result = { value: string };
type Results = Result[];

function isResponseInData<T>(value: T | { data: T}): value is { data: T } {
AssertType(value.hasOwnProperty('data'), "boolean");
AssertType(value.hasOwnProperty, "(PropertyKey) => boolean");
AssertType('data', "string");
    return value.hasOwnProperty('data');
}

function getResults1(value: Results | { data: Results }): Results {
AssertType(isResponseInData(value) ? value.data : value, "Results");
AssertType(isResponseInData(value), "boolean");
AssertType(isResponseInData, "<T>(union) => value is { data: T; }");
AssertType(value, "union");
AssertType(value.data, "Results");
AssertType(value, "Results");
    return isResponseInData(value) ? value.data : value;
}

function isPlainResponse<T>(value: T | { data: T}): value is T {
AssertType(!value.hasOwnProperty('data'), "boolean");
AssertType(value.hasOwnProperty('data'), "boolean");
AssertType(value.hasOwnProperty, "(PropertyKey) => boolean");
AssertType('data', "string");
    return !value.hasOwnProperty('data');
}

function getResults2(value: Results | { data: Results }): Results {
AssertType(isPlainResponse(value) ? value : value.data, "Results");
AssertType(isPlainResponse(value), "boolean");
AssertType(isPlainResponse, "<T>(union) => value is T");
AssertType(value, "union");
AssertType(value, "Results");
AssertType(value.data, "Results");
    return isPlainResponse(value) ? value : value.data;
}

