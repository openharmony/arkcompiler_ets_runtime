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

// === tests/cases/compiler/strictNullNotNullIndexTypeShouldWork.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    params?: { name: string; };
}

class Test<T extends A> {
    attrs: Readonly<T>;

    m() {
        this.attrs.params!.name;
AssertType(this.attrs.params!.name, "string");
AssertType(this.attrs.params!, "NonNullable<T["params"]>");
AssertType(this.attrs.params, "union");
AssertType(this.attrs, "Readonly<T>");
AssertType(this, "this");
    }
}

interface Foo {
    foo?: number;
}

class FooClass<P extends Foo = Foo> {
    properties: Readonly<P>;

    foo(): number {
        const { foo = 42 } = this.properties;
AssertType(foo, "number");
AssertType(42, "int");
AssertType(this.properties, "Readonly<P>");
AssertType(this, "this");

AssertType(foo, "number");
        return foo;
    }
}

class Test2<T extends A> {
    attrs: Readonly<T>;

    m() {
AssertType(this.attrs.params!, "NonNullable<T["params"]>");
AssertType(this.attrs.params, "union");
AssertType(this.attrs, "Readonly<T>");
AssertType(this, "this");
        return this.attrs.params!; // Return type should maintain relationship with `T` after being not-null-asserted, ideally
    }
}

