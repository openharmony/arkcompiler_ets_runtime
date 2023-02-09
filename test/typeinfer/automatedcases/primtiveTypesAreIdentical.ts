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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/primtiveTypesAreIdentical.ts ===
declare function AssertType(value:any, type:string):void;

// primitive types are identical to themselves so these overloads will all cause errors

function foo1(x: number);
function foo1(x: number);
function foo1(x: any) { }

function foo2(x: string);
function foo2(x: string);
function foo2(x: any) { }

function foo3(x: boolean);
function foo3(x: boolean);
function foo3(x: any) { }

function foo4(x: any);
function foo4(x: any);
function foo4(x: any) { }

function foo5(x: 'a');
function foo5(x: 'a');
function foo5(x: string);
function foo5(x: any) { }

enum E { A }
function foo6(x: E);
function foo6(x: E);
function foo6(x: any) { }

function foo7(x: void);
function foo7(x: void);
function foo7(x: any) { }

