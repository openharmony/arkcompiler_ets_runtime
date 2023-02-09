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

// === tests/cases/compiler/interfaceClassMerging2.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo {
    interfaceFooMethod(): this;
    interfaceFooProperty: this;
}

class Foo {
    classFooProperty: this;

    classFooMethod(): this {
AssertType(this, "this");
        return this;
    }
}


interface Bar {
    interfaceBarMethod(): this;
    interfaceBarProperty: this;
}

class Bar extends Foo {
    classBarProperty: this;

    classBarMethod(): this {
AssertType(this, "this");
        return this;
    }
}


let bar = new Bar();
AssertType(bar, "Bar");
AssertType(new Bar(), "Bar");
AssertType(Bar, "typeof Bar");

bar.interfaceBarMethod().interfaceFooMethod().classBarMethod().classFooMethod();
AssertType(bar.interfaceBarMethod().interfaceFooMethod().classBarMethod().classFooMethod(), "Bar");
AssertType(bar.interfaceBarMethod().interfaceFooMethod().classBarMethod().classFooMethod, "() => Bar");


let foo = new Foo();
AssertType(foo, "Foo");
AssertType(new Foo(), "Foo");
AssertType(Foo, "typeof Foo");

foo = bar;
AssertType(foo = bar, "Bar");
AssertType(foo, "Foo");
AssertType(bar, "Bar");


