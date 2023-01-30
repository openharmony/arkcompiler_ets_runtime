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

// === tests/cases/compiler/genericStaticAnyTypeFunction.ts ===
declare function AssertType(value:any, type:string):void;
class A {

    static one<T>(source: T, value: number): T {

AssertType(source, "T");
        return source;

    }
    static goo() { 
AssertType(0, "int");
return 0; 
}

    static two<T>(source: T): T {
AssertType(this.one<T>(source, 42), "T");
AssertType(this.one, "<T>(T, number) => T");
AssertType(this, "typeof A");
AssertType(source, "T");
AssertType(42, "int");
        return this.one<T>(source, 42); // should not error

    }

}




