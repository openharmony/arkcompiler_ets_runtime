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

// === tests/cases/conformance/types/contextualTypes/methodDeclarations/contextuallyTypedBindingInitializer.ts ===
declare function AssertType(value:any, type:string):void;
interface Show {
    show: (x: number) => string;
}
function f({ show = v => v.toString() }: Show) {}
function f2({ "show": showRename = v => v.toString() }: Show) {}
function f3({ ["show"]: showRename = v => v.toString() }: Show) {}

interface Nested {
    nested: Show
}
function ff({ nested = { show: v => v.toString() } }: Nested) {}

interface Tuples {
    prop: [string, number];
}
function g({ prop = ["hello", 1234] }: Tuples) {}

interface StringUnion {
    prop: "foo" | "bar";
}
function h({ prop = "foo" }: StringUnion) {}

interface StringIdentity {
    stringIdentity(s: string): string;
}
let { stringIdentity: id = arg => arg }: StringIdentity = { stringIdentity: x => x};
AssertType(stringIdentity, "any");
AssertType(id, "(string) => string");
AssertType(arg => arg, "(string) => string");
AssertType(arg, "string");
AssertType(arg, "string");
AssertType({ stringIdentity: x => x}, "{ stringIdentity: (string) => string; }");
AssertType(stringIdentity, "(string) => string");
AssertType(x => x, "(string) => string");
AssertType(x, "string");
AssertType(x, "string");




