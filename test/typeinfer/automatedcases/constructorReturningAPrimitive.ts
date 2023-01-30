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

// === tests/cases/compiler/constructorReturningAPrimitive.ts ===
declare function AssertType(value:any, type:string):void;
// technically not allowed by JavaScript but we don't have a 'not-primitive' constraint
// functionally only possible when your class is otherwise devoid of members so of little consequence in practice

class A {
    constructor() {
AssertType(1, "int");
        return 1;
    }
}

let a = new A();
AssertType(a, "A");
AssertType(new A(), "A");
AssertType(A, "typeof A");

class B<T> {
    constructor() {
        let x: T;
AssertType(x, "T");

AssertType(x, "T");
        return x;
    }
}

let b = new B<number>();
AssertType(b, "B<number>");
AssertType(new B<number>(), "B<number>");
AssertType(B, "typeof B");


