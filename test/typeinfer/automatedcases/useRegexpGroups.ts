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

// === tests/cases/conformance/es2018/useRegexpGroups.ts ===
declare function AssertType(value:any, type:string):void;
let re = /(?<year>\d{4})-(?<month>\d{2})-(?<day>\d{2})/u;
AssertType(re, "RegExp");
AssertType(/(?<year>\d{4})-(?<month>\d{2})-(?<day>\d{2})/u, "RegExp");

let result = re.exec("2015-01-02");
AssertType(result, "RegExpExecArray");
AssertType(re.exec("2015-01-02"), "RegExpExecArray");
AssertType(re.exec, "(string) => RegExpExecArray");
AssertType("2015-01-02", "string");

let date = result[0];
AssertType(date, "string");
AssertType(result[0], "string");
AssertType(result, "RegExpExecArray");
AssertType(0, "int");

let year1 = result.groups.year;
AssertType(year1, "string");
AssertType(result.groups.year, "string");

let year2 = result[1];
AssertType(year2, "string");
AssertType(result[1], "string");
AssertType(result, "RegExpExecArray");
AssertType(1, "int");

let month1 = result.groups.month;
AssertType(month1, "string");
AssertType(result.groups.month, "string");

let month2 = result[2];
AssertType(month2, "string");
AssertType(result[2], "string");
AssertType(result, "RegExpExecArray");
AssertType(2, "int");

let day1 = result.groups.day;
AssertType(day1, "string");
AssertType(result.groups.day, "string");

let day2 = result[3];
AssertType(day2, "string");
AssertType(result[3], "string");
AssertType(result, "RegExpExecArray");
AssertType(3, "int");

let foo = "foo".match(/(?<bar>foo)/)!.groups.foo;
AssertType(foo, "string");
AssertType("foo".match(/(?<bar>foo)/)!.groups.foo, "string");


