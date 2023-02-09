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

// === tests/cases/compiler/ambientModuleWithTemplateLiterals.ts ===
declare function AssertType(value:any, type:string):void;
declare module Foo {
    enum Bar {
        a = `1`,
        b = '2',
        c = '3'
    }

    export const a = 'string';
    export const b = `template`;

    export const c = Bar.a;
    export const d = Bar['b'];
    export const e = Bar[`c`];
}

Foo.a;
AssertType(Foo.a, "string");

Foo.b;
AssertType(Foo.b, "string");

Foo.c;
AssertType(Foo.c, "Foo.Bar.a");

Foo.d;
AssertType(Foo.d, "Foo.Bar.b");

Foo.e;
AssertType(Foo.e, "Foo.Bar.c");


