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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/typeParameterAsTypeArgument.ts ===
declare function AssertType(value:any, type:string):void;
// These are all errors because type parameters cannot reference other type parameters from the same list

function foo<T, U>(x: T, y: U) {
    foo<U, U>(y, y);
AssertType(foo<U, U>(y, y), "C<U, U>");
AssertType(foo, "<T, U>(T, U) => C<U, T>");
AssertType(y, "U");
AssertType(y, "U");

AssertType(new C<U,T>(), "C<U, T>");
AssertType(C, "typeof C");
    return new C<U,T>();
}

class C<T, U> {
    x: T;
}

interface I<T, U> {
    x: C<U, T>;
}


