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

// === tests/cases/compiler/controlFlowOuterVariable.ts ===
declare function AssertType(value:any, type:string):void;
// Repros from #10641

const CONFIG = {
AssertType(CONFIG, "{ foo: string; setFoo: (string) => void; }");
AssertType({    foo: '',    setFoo: function(foo: string) {        CONFIG.foo = foo;    }}, "{ foo: string; setFoo: (string) => void; }");

    foo: '',
AssertType(foo, "string");
AssertType('', "string");

    setFoo: function(foo: string) {
AssertType(setFoo, "(string) => void");
AssertType(function(foo: string) {        CONFIG.foo = foo;    }, "(string) => void");
AssertType(foo, "string");

        CONFIG.foo = foo;
AssertType(CONFIG.foo = foo, "string");
AssertType(CONFIG.foo, "string");
AssertType(foo, "string");
    }
};

const helper = function<T>(t: T[]) {
AssertType(helper, "<T>(T[]) => void");
AssertType(function<T>(t: T[]) {    helper(t.slice(1));}, "<T>(T[]) => void");
AssertType(t, "T[]");

    helper(t.slice(1));
AssertType(helper(t.slice(1)), "void");
AssertType(helper, "<T>(T[]) => void");
AssertType(t.slice(1), "T[]");
AssertType(t.slice, "(?union, ?union) => T[]");
AssertType(1, "int");
}

