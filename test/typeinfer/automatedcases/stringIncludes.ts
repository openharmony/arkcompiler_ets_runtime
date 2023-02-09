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

// === tests/cases/compiler/stringIncludes.ts ===
declare function AssertType(value:any, type:string):void;
let includes: boolean;
AssertType(includes, "boolean");

includes = "abcde".includes("cd");
AssertType(includes = "abcde".includes("cd"), "boolean");
AssertType(includes, "boolean");
AssertType("abcde".includes("cd"), "boolean");
AssertType("abcde".includes, "(string, ?number) => boolean");
AssertType("cd", "string");

includes = "abcde".includes("cd", 2);
AssertType(includes = "abcde".includes("cd", 2), "boolean");
AssertType(includes, "boolean");
AssertType("abcde".includes("cd", 2), "boolean");
AssertType("abcde".includes, "(string, ?number) => boolean");
AssertType("cd", "string");
AssertType(2, "int");


