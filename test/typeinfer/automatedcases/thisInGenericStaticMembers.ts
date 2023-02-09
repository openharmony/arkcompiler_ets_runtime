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

// === tests/cases/compiler/thisInGenericStaticMembers.ts ===
declare function AssertType(value:any, type:string):void;
// this.call in static generic method not resolved correctly

class A {

    static one<T>(source:T, value: number): T {
AssertType(source, "T");
        return source;
    }

    static two<T>(source: T): T {
AssertType(this.one<T>(source, 42), "T");
AssertType(this.one, "<T>(T, number) => T");
AssertType(this, "typeof A");
AssertType(source, "T");
AssertType(42, "int");
        return this.one<T>(source, 42);
    }
}

class B {

    static one(source: B, value: number): B {
AssertType(source, "B");
        return source;
    }

    static two(source: B): B {
AssertType(this.one(source, 42), "B");
AssertType(this.one, "(B, number) => B");
AssertType(this, "typeof B");
AssertType(source, "B");
AssertType(42, "int");
        return this.one(source, 42);
    }
}




