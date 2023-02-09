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

// === tests/cases/conformance/salsa/b.ts ===
declare function AssertType(value:any, type:string):void;
variable.a = 1;
AssertType(variable.a = 1, "int");
AssertType(variable.a, "number");
AssertType(1, "int");

(new C()).member.a = 1;
AssertType((new C()).member.a = 1, "int");
AssertType((new C()).member.a, "any");
AssertType(1, "int");

(new C()).initializedMember.a = 1;
AssertType((new C()).initializedMember.a = 1, "int");
AssertType((new C()).initializedMember.a, "any");
AssertType(1, "int");

obj.property.a = 1;
AssertType(obj.property.a = 1, "int");
AssertType(obj.property.a, "any");
AssertType(1, "int");

arr[0].a = 1;
AssertType(arr[0].a = 1, "int");
AssertType(arr[0].a, "any");
AssertType(1, "int");

getObj().a = 1;
AssertType(getObj().a = 1, "int");
AssertType(getObj().a, "any");
AssertType(1, "int");



