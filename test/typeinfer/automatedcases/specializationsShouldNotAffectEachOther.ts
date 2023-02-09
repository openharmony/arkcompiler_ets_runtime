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

// === tests/cases/compiler/specializationsShouldNotAffectEachOther.ts ===
declare function AssertType(value:any, type:string):void;
interface Series  {
    data: string[];
}

let series: Series;
AssertType(series, "Series");


function foo() {

    let seriesExtent = (series) => null;
AssertType(seriesExtent, "(any) => any");
AssertType((series) => null, "(any) => any");
AssertType(series, "any");
AssertType(null, "null");

    let series2: number[];
AssertType(series2, "number[]");

    series2.map(seriesExtent);
AssertType(series2.map(seriesExtent), "any[]");
AssertType(series2.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(seriesExtent, "(any) => any");

AssertType(null, "null");
    return null;
}


let keyExtent2: any[] = series.data.map(function (d: string) { 
AssertType(keyExtent2, "any[]");
AssertType(series.data.map(function (d: string) { return d; }), "string[]");
AssertType(series.data.map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(function (d: string) { return d; }, "(string) => string");
AssertType(d, "string");
AssertType(d, "string");
return d; });


