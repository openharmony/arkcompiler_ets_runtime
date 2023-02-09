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

// === tests/cases/compiler/taggedTemplatesInDifferentScopes.ts ===
declare function AssertType(value:any, type:string):void;
export function tag(parts: TemplateStringsArray, ...values: any[]) {
AssertType(parts[0], "string");
AssertType(parts, "TemplateStringsArray");
AssertType(0, "int");
    return parts[0];
}
function foo() {
    tag `foo`;
AssertType(tag `foo`, "string");
AssertType(tag, "(TemplateStringsArray, ...any[]) => string");
AssertType(`foo`, "string");

    tag `foo2`;
AssertType(tag `foo2`, "string");
AssertType(tag, "(TemplateStringsArray, ...any[]) => string");
AssertType(`foo2`, "string");
}

function bar() {
    tag `bar`;
AssertType(tag `bar`, "string");
AssertType(tag, "(TemplateStringsArray, ...any[]) => string");
AssertType(`bar`, "string");

    tag `bar2`;
AssertType(tag `bar2`, "string");
AssertType(tag, "(TemplateStringsArray, ...any[]) => string");
AssertType(`bar2`, "string");
}

foo();
AssertType(foo(), "void");
AssertType(foo, "() => void");

bar();
AssertType(bar(), "void");
AssertType(bar, "() => void");


