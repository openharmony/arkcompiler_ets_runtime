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

// === tests/cases/compiler/library_StringSlice.ts ===
declare function AssertType(value:any, type:string):void;
// String.prototype.slice can have zero, one, or two arguments
String.prototype.slice();
AssertType(String.prototype.slice(), "string");
AssertType(String.prototype.slice, "(?number, ?number) => string");

String.prototype.slice(0);
AssertType(String.prototype.slice(0), "string");
AssertType(String.prototype.slice, "(?number, ?number) => string");
AssertType(0, "int");

String.prototype.slice(0,1);
AssertType(String.prototype.slice(0,1), "string");
AssertType(String.prototype.slice, "(?number, ?number) => string");
AssertType(0, "int");
AssertType(1, "int");


