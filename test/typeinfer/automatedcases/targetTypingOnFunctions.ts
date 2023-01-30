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

// === tests/cases/compiler/targetTypingOnFunctions.ts ===
declare function AssertType(value:any, type:string):void;
let fu: (s: string) => string = function (s) { 
AssertType(fu, "(string) => string");
AssertType(s, "string");
AssertType(function (s) { return s.toLowerCase() }, "(string) => string");
AssertType(s, "string");
AssertType(s.toLowerCase(), "string");
AssertType(s.toLowerCase, "() => string");
return s.toLowerCase() };

let zu = fu = function (s) { 
AssertType(zu, "(string) => string");
AssertType(fu = function (s) { return s.toLowerCase() }, "(string) => string");
AssertType(fu, "(string) => string");
AssertType(function (s) { return s.toLowerCase() }, "(string) => string");
AssertType(s, "string");
AssertType(s.toLowerCase(), "string");
AssertType(s.toLowerCase, "() => string");
return s.toLowerCase() };


