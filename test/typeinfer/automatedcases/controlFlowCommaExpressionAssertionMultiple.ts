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

// === tests/cases/compiler/controlFlowCommaExpressionAssertionMultiple.ts ===
declare function AssertType(value:any, type:string):void;
function Narrow<T>(value: any): asserts value is T {}

function func(foo: any, bar: any) {
    Narrow<number>(foo), Narrow<string>(bar);
AssertType(Narrow<number>(foo), Narrow<string>(bar), "void");
AssertType(Narrow<number>(foo), "void");
AssertType(Narrow, "<T>(any) => asserts value is T");
AssertType(foo, "any");
AssertType(Narrow<string>(bar), "void");
AssertType(Narrow, "<T>(any) => asserts value is T");
AssertType(bar, "any");

    foo;
AssertType(foo, "number");

    bar;
AssertType(bar, "string");
}

function func2(foo: any, bar: any, baz: any) {
    Narrow<number>(foo), Narrow<string>(bar), Narrow<boolean>(baz);
AssertType(Narrow<number>(foo), Narrow<string>(bar), Narrow<boolean>(baz), "void");
AssertType(Narrow<number>(foo), Narrow<string>(bar), "void");
AssertType(Narrow<number>(foo), "void");
AssertType(Narrow, "<T>(any) => asserts value is T");
AssertType(foo, "any");
AssertType(Narrow<string>(bar), "void");
AssertType(Narrow, "<T>(any) => asserts value is T");
AssertType(bar, "any");
AssertType(Narrow<boolean>(baz), "void");
AssertType(Narrow, "<T>(any) => asserts value is T");
AssertType(baz, "any");

    foo;
AssertType(foo, "number");

    bar;
AssertType(bar, "string");

    baz;
AssertType(baz, "boolean");
}


