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

// === tests/cases/conformance/classes/classStaticBlock/classStaticBlock27.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/44872

void class Foo {
AssertType(void class Foo {    static prop = 1    static {        console.log(Foo.prop);        Foo.prop++;    }    static {        console.log(Foo.prop);        Foo.prop++;    }    static {        console.log(Foo.prop);        Foo.prop++;    }}, "undefined");
AssertType(class Foo {    static prop = 1    static {        console.log(Foo.prop);        Foo.prop++;    }    static {        console.log(Foo.prop);        Foo.prop++;    }    static {        console.log(Foo.prop);        Foo.prop++;    }}, "typeof Foo");
AssertType(Foo, "typeof Foo");

    static prop = 1
AssertType(prop, "number");
AssertType(1, "int");

    static {
        console.log(Foo.prop);
AssertType(console.log(Foo.prop), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(Foo.prop, "number");

        Foo.prop++;
AssertType(Foo.prop++, "number");
AssertType(Foo.prop, "number");
    }
    static {
        console.log(Foo.prop);
AssertType(console.log(Foo.prop), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(Foo.prop, "number");

        Foo.prop++;
AssertType(Foo.prop++, "number");
AssertType(Foo.prop, "number");
    }
    static {
        console.log(Foo.prop);
AssertType(console.log(Foo.prop), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(Foo.prop, "number");

        Foo.prop++;
AssertType(Foo.prop++, "number");
AssertType(Foo.prop, "number");
    }
}

