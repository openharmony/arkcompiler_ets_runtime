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

// === tests/cases/compiler/file0.ts ===
declare function AssertType(value:any, type:string):void;
/// <reference path="file1.ts" />
/// <reference path="file2.ts" />
/// <amd-dependency path="/js/libs/hgn.js!app/templates/home" name="compiler"/>
let x = 10;
AssertType(x, "number");
AssertType(10, "int");

/// <reference path="file1.ts" />
let y = "hello";
AssertType(y, "string");
AssertType("hello", "string");


/// <reference path="file2.ts" />


