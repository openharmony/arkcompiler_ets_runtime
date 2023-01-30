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

// === tests/cases/compiler/circularObjectLiteralAccessors.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #6000

const a = {
AssertType(a, "{ b: { foo: string; }; foo: string; }");
AssertType({    b: {        get foo(): string {            return a.foo;        },        set foo(value: string) {            a.foo = value;        }    },    foo: ''}, "{ b: { foo: string; }; foo: string; }");

    b: {
AssertType(b, "{ foo: string; }");
AssertType({        get foo(): string {            return a.foo;        },        set foo(value: string) {            a.foo = value;        }    }, "{ foo: string; }");

        get foo(): string {
AssertType(foo, "string");

AssertType(a.foo, "string");
            return a.foo;

        },
        set foo(value: string) {
AssertType(foo, "string");
AssertType(value, "string");

            a.foo = value;
AssertType(a.foo = value, "string");
AssertType(a.foo, "string");
AssertType(value, "string");
        }
    },
    foo: ''
AssertType(foo, "string");
AssertType('', "string");

};

