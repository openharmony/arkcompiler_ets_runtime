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

// === tests/cases/compiler/library_DatePrototypeProperties.ts ===
declare function AssertType(value:any, type:string):void;
// Properties of the Date prototype object as per ES5 spec
// http://www.ecma-international.org/ecma-262/5.1/#sec-15.9.5
Date.prototype.constructor;
AssertType(Date.prototype.constructor, "Function");

Date.prototype.toString();
AssertType(Date.prototype.toString(), "string");
AssertType(Date.prototype.toString, "() => string");

Date.prototype.toDateString();
AssertType(Date.prototype.toDateString(), "string");
AssertType(Date.prototype.toDateString, "() => string");

Date.prototype.toTimeString();
AssertType(Date.prototype.toTimeString(), "string");
AssertType(Date.prototype.toTimeString, "() => string");

Date.prototype.toLocaleString();
AssertType(Date.prototype.toLocaleString(), "string");
AssertType(Date.prototype.toLocaleString, "{ (): string; (?union, ?Intl.DateTimeFormatOptions): string; }");

Date.prototype.toLocaleDateString();
AssertType(Date.prototype.toLocaleDateString(), "string");
AssertType(Date.prototype.toLocaleDateString, "{ (): string; (?union, ?Intl.DateTimeFormatOptions): string; }");

Date.prototype.toLocaleTimeString();
AssertType(Date.prototype.toLocaleTimeString(), "string");
AssertType(Date.prototype.toLocaleTimeString, "{ (): string; (?union, ?Intl.DateTimeFormatOptions): string; }");

Date.prototype.valueOf();
AssertType(Date.prototype.valueOf(), "number");
AssertType(Date.prototype.valueOf, "() => number");

Date.prototype.getTime();
AssertType(Date.prototype.getTime(), "number");
AssertType(Date.prototype.getTime, "() => number");

Date.prototype.getFullYear();
AssertType(Date.prototype.getFullYear(), "number");
AssertType(Date.prototype.getFullYear, "() => number");

Date.prototype.getUTCFullYear();
AssertType(Date.prototype.getUTCFullYear(), "number");
AssertType(Date.prototype.getUTCFullYear, "() => number");

Date.prototype.getMonth();
AssertType(Date.prototype.getMonth(), "number");
AssertType(Date.prototype.getMonth, "() => number");

Date.prototype.getUTCMonth();
AssertType(Date.prototype.getUTCMonth(), "number");
AssertType(Date.prototype.getUTCMonth, "() => number");

Date.prototype.getDate();
AssertType(Date.prototype.getDate(), "number");
AssertType(Date.prototype.getDate, "() => number");

Date.prototype.getUTCDate();
AssertType(Date.prototype.getUTCDate(), "number");
AssertType(Date.prototype.getUTCDate, "() => number");

Date.prototype.getDay();
AssertType(Date.prototype.getDay(), "number");
AssertType(Date.prototype.getDay, "() => number");

Date.prototype.getUTCDay();
AssertType(Date.prototype.getUTCDay(), "number");
AssertType(Date.prototype.getUTCDay, "() => number");

Date.prototype.getHours();
AssertType(Date.prototype.getHours(), "number");
AssertType(Date.prototype.getHours, "() => number");

Date.prototype.getUTCHours();
AssertType(Date.prototype.getUTCHours(), "number");
AssertType(Date.prototype.getUTCHours, "() => number");

Date.prototype.getMinutes();
AssertType(Date.prototype.getMinutes(), "number");
AssertType(Date.prototype.getMinutes, "() => number");

Date.prototype.getUTCMinutes();
AssertType(Date.prototype.getUTCMinutes(), "number");
AssertType(Date.prototype.getUTCMinutes, "() => number");

Date.prototype.getSeconds();
AssertType(Date.prototype.getSeconds(), "number");
AssertType(Date.prototype.getSeconds, "() => number");

Date.prototype.getUTCSeconds();
AssertType(Date.prototype.getUTCSeconds(), "number");
AssertType(Date.prototype.getUTCSeconds, "() => number");

Date.prototype.getMilliseconds();
AssertType(Date.prototype.getMilliseconds(), "number");
AssertType(Date.prototype.getMilliseconds, "() => number");

Date.prototype.getUTCMilliseconds();
AssertType(Date.prototype.getUTCMilliseconds(), "number");
AssertType(Date.prototype.getUTCMilliseconds, "() => number");

Date.prototype.getTimezoneOffset();
AssertType(Date.prototype.getTimezoneOffset(), "number");
AssertType(Date.prototype.getTimezoneOffset, "() => number");

Date.prototype.setTime(0);
AssertType(Date.prototype.setTime(0), "number");
AssertType(Date.prototype.setTime, "(number) => number");
AssertType(0, "int");

Date.prototype.setMilliseconds(0);
AssertType(Date.prototype.setMilliseconds(0), "number");
AssertType(Date.prototype.setMilliseconds, "(number) => number");
AssertType(0, "int");

Date.prototype.setUTCMilliseconds(0);
AssertType(Date.prototype.setUTCMilliseconds(0), "number");
AssertType(Date.prototype.setUTCMilliseconds, "(number) => number");
AssertType(0, "int");

Date.prototype.setSeconds(0);
AssertType(Date.prototype.setSeconds(0), "number");
AssertType(Date.prototype.setSeconds, "(number, ?number) => number");
AssertType(0, "int");

Date.prototype.setUTCSeconds(0);
AssertType(Date.prototype.setUTCSeconds(0), "number");
AssertType(Date.prototype.setUTCSeconds, "(number, ?number) => number");
AssertType(0, "int");

Date.prototype.setMinutes(0);
AssertType(Date.prototype.setMinutes(0), "number");
AssertType(Date.prototype.setMinutes, "(number, ?number, ?number) => number");
AssertType(0, "int");

Date.prototype.setUTCMinutes(0);
AssertType(Date.prototype.setUTCMinutes(0), "number");
AssertType(Date.prototype.setUTCMinutes, "(number, ?number, ?number) => number");
AssertType(0, "int");

Date.prototype.setHours(0);
AssertType(Date.prototype.setHours(0), "number");
AssertType(Date.prototype.setHours, "(number, ?number, ?number, ?number) => number");
AssertType(0, "int");

Date.prototype.setUTCHours(0);
AssertType(Date.prototype.setUTCHours(0), "number");
AssertType(Date.prototype.setUTCHours, "(number, ?number, ?number, ?number) => number");
AssertType(0, "int");

Date.prototype.setDate(0);
AssertType(Date.prototype.setDate(0), "number");
AssertType(Date.prototype.setDate, "(number) => number");
AssertType(0, "int");

Date.prototype.setUTCDate(0);
AssertType(Date.prototype.setUTCDate(0), "number");
AssertType(Date.prototype.setUTCDate, "(number) => number");
AssertType(0, "int");

Date.prototype.setMonth(0);
AssertType(Date.prototype.setMonth(0), "number");
AssertType(Date.prototype.setMonth, "(number, ?number) => number");
AssertType(0, "int");

Date.prototype.setUTCMonth(0);
AssertType(Date.prototype.setUTCMonth(0), "number");
AssertType(Date.prototype.setUTCMonth, "(number, ?number) => number");
AssertType(0, "int");

Date.prototype.setFullYear(0);
AssertType(Date.prototype.setFullYear(0), "number");
AssertType(Date.prototype.setFullYear, "(number, ?number, ?number) => number");
AssertType(0, "int");

Date.prototype.setUTCFullYear(0);
AssertType(Date.prototype.setUTCFullYear(0), "number");
AssertType(Date.prototype.setUTCFullYear, "(number, ?number, ?number) => number");
AssertType(0, "int");

Date.prototype.toUTCString();
AssertType(Date.prototype.toUTCString(), "string");
AssertType(Date.prototype.toUTCString, "() => string");

Date.prototype.toISOString();
AssertType(Date.prototype.toISOString(), "string");
AssertType(Date.prototype.toISOString, "() => string");

Date.prototype.toJSON(null);
AssertType(Date.prototype.toJSON(null), "string");
AssertType(Date.prototype.toJSON, "(?any) => string");
AssertType(null, "null");


