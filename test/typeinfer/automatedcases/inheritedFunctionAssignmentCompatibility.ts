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

// === tests/cases/compiler/inheritedFunctionAssignmentCompatibility.ts ===
declare function AssertType(value:any, type:string):void;
interface IResultCallback extends Function { }

function fn(cb: IResultCallback) { }

fn((a, b) => true);
AssertType(fn((a, b) => true), "void");
AssertType(fn, "(IResultCallback) => void");
AssertType((a, b) => true, "(any, any) => boolean");
AssertType(a, "any");
AssertType(b, "any");
AssertType(true, "boolean");

fn(function (a, b) { 
AssertType(fn(function (a, b) { return true; }), "void");
AssertType(fn, "(IResultCallback) => void");
AssertType(function (a, b) { return true; }, "(any, any) => boolean");
AssertType(a, "any");
AssertType(b, "any");
AssertType(true, "boolean");
return true; })



