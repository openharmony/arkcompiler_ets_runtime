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

// === tests/cases/compiler/conditionalEqualityTestingNullability.ts ===
declare function AssertType(value:any, type:string):void;
export type Equals<A1 extends any, A2 extends any> =
    (<A>() => A extends A1 ? 1 : 0) extends (<A>() => A extends A2 ? 1 : 0)
    ? 1
    : 0

interface Foo<T> {
    x : () => T
}

declare const a: Foo<Date>;
AssertType(a, "Foo<Date>");

declare const b: Foo<Date | null>;
AssertType(b, "Foo<union>");
AssertType(null, "null");

//Expected 0, Actual 1
type ShouldBe0 = Equals<typeof a, typeof b>;

