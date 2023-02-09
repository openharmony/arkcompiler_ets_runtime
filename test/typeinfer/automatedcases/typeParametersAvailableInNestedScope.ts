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

// === tests/cases/conformance/types/typeParameters/typeParameterLists/typeParametersAvailableInNestedScope.ts ===
declare function AssertType(value:any, type:string):void;
class C<T> {
    data: T;

    x = <U>(a: U) => {
        let y: T;
AssertType(y, "T");

AssertType(y, "T");
        return y;
    }

    foo() {
        function temp<U>(a: U) {
AssertType(temp, "<U>(U) => T");
AssertType(a, "U");

            let y: T;
AssertType(y, "T");

AssertType(y, "T");
            return y;
        }
AssertType(temp(<T>null), "T");
AssertType(temp, "<U>(U) => T");
AssertType(<T>null, "T");
AssertType(null, "null");
        return temp(<T>null);
    }
}

let c = new C<number>();
AssertType(c, "C<number>");
AssertType(new C<number>(), "C<number>");
AssertType(C, "typeof C");

c.data = c.x(null);
AssertType(c.data = c.x(null), "number");
AssertType(c.data, "number");
AssertType(c.x(null), "number");
AssertType(c.x, "<U>(U) => number");
AssertType(null, "null");

c.data = c.foo();
AssertType(c.data = c.foo(), "number");
AssertType(c.data, "number");
AssertType(c.foo(), "number");
AssertType(c.foo, "() => number");


