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

// === tests/cases/conformance/salsa/propertyAssignmentUseParentType3.ts ===
declare function AssertType(value:any, type:string):void;
// don't use the parent type if it's a function declaration (#33741)

function foo1(): number {
AssertType(123, "int");
    return 123;
}
foo1.toFixed = "";
AssertType(foo1.toFixed = "", "string");
AssertType(foo1.toFixed, "string");
AssertType("", "string");

function foo2(): any[] {
AssertType([], "undefined[]");
    return [];
}
foo2.join = "";
AssertType(foo2.join = "", "string");
AssertType(foo2.join, "string");
AssertType("", "string");

function foo3(): string {
AssertType("", "string");
    return "";
}
foo3.trim = "";
AssertType(foo3.trim = "", "string");
AssertType(foo3.trim, "string");
AssertType("", "string");

function foo4(): ({x: number}) {
AssertType({x: 123}, "{ x: number; }");
AssertType(x, "number");
AssertType(123, "int");
    return {x: 123};
}
foo4.x = "456";
AssertType(foo4.x = "456", "string");
AssertType(foo4.x, "string");
AssertType("456", "string");


