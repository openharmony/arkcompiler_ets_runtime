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

// === tests/cases/compiler/classExpressionTest2.ts ===
declare function AssertType(value:any, type:string):void;
function M() {
    let m = class C<X> {
AssertType(m, "typeof C");
AssertType(C, "typeof C");
AssertType(class C<X> {        f<T>() {            let t: T;            let x: X;            return { t, x };        }    }, "typeof C");

        f<T>() {
AssertType(f, "<T>() => { t: T; x: X; }");

            let t: T;
AssertType(t, "T");

            let x: X;
AssertType(x, "X");

AssertType({ t, x }, "{ t: T; x: X; }");
AssertType(t, "T");
AssertType(x, "X");
            return { t, x };
        }
    }

    let v = new m<number>();
AssertType(v, "C<number>");
AssertType(new m<number>(), "C<number>");
AssertType(m, "typeof C");

AssertType(v.f<string>(), "{ t: string; x: number; }");
AssertType(v.f, "<T>() => { t: T; x: number; }");
    return v.f<string>();
}

