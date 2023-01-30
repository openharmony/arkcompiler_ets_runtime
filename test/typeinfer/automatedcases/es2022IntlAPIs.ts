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

// === tests/cases/conformance/es2022/es2022IntlAPIs.ts ===
declare function AssertType(value:any, type:string):void;
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Intl/DateTimeFormat/DateTimeFormat#using_timezonename
const timezoneNames = ['short', 'long', 'shortOffset', 'longOffset', 'shortGeneric', 'longGeneric'] as const;
AssertType(timezoneNames, "readonly ["short", "long", "shortOffset", "longOffset", "shortGeneric", "longGeneric"]");
AssertType(['short', 'long', 'shortOffset', 'longOffset', 'shortGeneric', 'longGeneric'] as const, "readonly ["short", "long", "shortOffset", "longOffset", "shortGeneric", "longGeneric"]");
AssertType(['short', 'long', 'shortOffset', 'longOffset', 'shortGeneric', 'longGeneric'], "readonly ["short", "long", "shortOffset", "longOffset", "shortGeneric", "longGeneric"]");
AssertType('short', "string");
AssertType('long', "string");
AssertType('shortOffset', "string");
AssertType('longOffset', "string");
AssertType('shortGeneric', "string");
AssertType('longGeneric', "string");

for (const zoneName of timezoneNames) {
  let formatter = new Intl.DateTimeFormat('en-US', {
AssertType(formatter, "Intl.DateTimeFormat");
AssertType(new Intl.DateTimeFormat('en-US', {    timeZone: 'America/Los_Angeles',    timeZoneName: zoneName,  }), "Intl.DateTimeFormat");
AssertType(Intl.DateTimeFormat, "{ (?union, ?Intl.DateTimeFormatOptions): Intl.DateTimeFormat; new (?union, ?Intl.DateTimeFormatOptions): Intl.DateTimeFormat; supportedLocalesOf(union, ?Intl.DateTimeFormatOptions): string[]; readonly prototype: Intl.DateTimeFormat; }");
AssertType('en-US', "string");
AssertType({    timeZone: 'America/Los_Angeles',    timeZoneName: zoneName,  }, "{ timeZone: string; timeZoneName: union; }");

    timeZone: 'America/Los_Angeles',
AssertType(timeZone, "string");
AssertType('America/Los_Angeles', "string");

    timeZoneName: zoneName,
AssertType(timeZoneName, "union");
AssertType(zoneName, "union");

  });
}


