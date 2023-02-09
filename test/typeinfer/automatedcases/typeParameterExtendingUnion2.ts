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

// === tests/cases/compiler/typeParameterExtendingUnion2.ts ===
declare function AssertType(value:any, type:string):void;
class Animal { run() { } }
class Cat extends Animal { meow }
class Dog extends Animal { woof }

function run(a: Cat | Dog) {
    a.run();
AssertType(a.run(), "void");
AssertType(a.run, "union");
}

function f<T extends Cat | Dog>(a: T) {
    a.run();
AssertType(a.run(), "void");
AssertType(a.run, "union");

    run(a);
AssertType(run(a), "void");
AssertType(run, "(union) => void");
AssertType(a, "union");
}

