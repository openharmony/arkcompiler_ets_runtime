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

// === tests/cases/conformance/types/localTypes/localTypes5.ts ===
declare function AssertType(value:any, type:string):void;
function foo<A>() {
    class X {
        m<B, C>() {
AssertType((function <D>() {                class Y<E> {                }                return new Y<string>();            })<Date>(), "(Anonymous function)<Date>.Y<string>");
AssertType((function <D>() {                class Y<E> {                }                return new Y<string>();            }), "<D>() => Y<string>");
AssertType(function <D>() {                class Y<E> {                }                return new Y<string>();            }, "<D>() => Y<string>");
            return (function <D>() {

                class Y<E> {
                }
AssertType(new Y<string>(), "Y<string>");
AssertType(Y, "typeof Y");
                return new Y<string>();

            })<Date>();
        }
    }
    let x = new X();
AssertType(x, "X");
AssertType(new X(), "X");
AssertType(X, "typeof X");

AssertType(x.m<number, boolean>(), "X.m<number, boolean>.(Anonymous function)<Date>.Y<string>");
AssertType(x.m, "<B, C>() => X.m<B, C>.(Anonymous function)<Date>.Y<string>");
    return x.m<number, boolean>();
}
let x = foo<void>();
AssertType(x, "foo<void>.X.m<number, boolean>.(Anonymous function)<Date>.Y<string>");
AssertType(foo<void>(), "foo<void>.X.m<number, boolean>.(Anonymous function)<Date>.Y<string>");
AssertType(foo, "<A>() => X.m<number, boolean>.(Anonymous function)<Date>.Y<string>");


