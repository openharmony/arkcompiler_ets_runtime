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

// === tests/cases/conformance/types/thisType/thisTypeAndConstraints.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    self() {
AssertType(this, "this");
        return this;
    }
}

function f<T extends A>(x: T) {
    function g<U extends T>(x: U) {
AssertType(g, "<U extends T>(U) => void");
AssertType(x, "U");

        x = x.self();
AssertType(x = x.self(), "U");
AssertType(x, "U");
AssertType(x.self(), "U");
AssertType(x.self, "() => U");
    }
    x = x.self();
AssertType(x = x.self(), "T");
AssertType(x, "T");
AssertType(x.self(), "T");
AssertType(x.self, "() => T");
}

class B<T extends A> {
    foo(x: T) {
        x = x.self();
AssertType(x = x.self(), "T");
AssertType(x, "T");
AssertType(x.self(), "T");
AssertType(x.self, "() => T");
    }
    bar<U extends T>(x: U) {
        x = x.self();
AssertType(x = x.self(), "U");
AssertType(x, "U");
AssertType(x.self(), "U");
AssertType(x.self, "() => U");
    }
}


