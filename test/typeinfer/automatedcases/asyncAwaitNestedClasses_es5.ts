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

// === tests/cases/conformance/async/es5/asyncAwaitNestedClasses_es5.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/Microsoft/TypeScript/issues/20744
class A {
    static B = class B {
        static func2(): Promise<void> {
AssertType(new Promise((resolve) => { resolve(null); }), "Promise<void>");
AssertType(Promise, "PromiseConstructor");
AssertType((resolve) => { resolve(null); }, "((union) => void) => void");
AssertType(resolve, "(union) => void");
AssertType(resolve(null), "void");
AssertType(resolve, "(union) => void");
AssertType(null, "null");
            return new Promise((resolve) => { resolve(null); });
        }
        static C = class C {
            static async func() {
                await B.func2();
AssertType(await B.func2(), "void");
AssertType(B.func2(), "Promise<void>");
AssertType(B.func2, "() => Promise<void>");
            }
        }
    }
}

A.B.C.func();
AssertType(A.B.C.func(), "Promise<void>");
AssertType(A.B.C.func, "() => Promise<void>");


