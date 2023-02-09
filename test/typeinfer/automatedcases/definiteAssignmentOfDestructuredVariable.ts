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

// === tests/cases/compiler/definiteAssignmentOfDestructuredVariable.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/Microsoft/TypeScript/issues/20994
interface Options {
    a?: number | object;
    b: () => void;
}

class C<T extends Options> {
    foo!: { [P in keyof T]: T[P] }

    method() {
        let { a, b } = this.foo;
AssertType(a, "union");
AssertType(b, "T["b"]");
AssertType(this.foo, "{ [P in keyof T]: T[P]; }");
AssertType(this, "this");

        !(a && b);
AssertType(!(a && b), "boolean");
AssertType((a && b), "union");
AssertType(a && b, "union");
AssertType(a, "union");
AssertType(b, "T["b"]");

        a;
AssertType(a, "union");
    }
}

