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

// === tests/cases/compiler/destructuredMaappedTypeIsNotImplicitlyAny.ts ===
declare function AssertType(value:any, type:string):void;
function foo<T extends string>(key: T, obj: { [_ in T]: number }) {
    const { [key]: bar } = obj; // Element implicitly has an 'any' type because type '{ [_ in T]: number; }' has no index signature.
AssertType(key, "T");
AssertType(bar, "{ [_ in T]: number; }[T]");
AssertType(obj, "{ [_ in T]: number; }");

    bar; // bar : any
AssertType(bar, "{ [_ in T]: number; }[T]");

    // Note: this does work:
    const lorem = obj[key];
AssertType(lorem, "{ [_ in T]: number; }[T]");
AssertType(obj[key], "{ [_ in T]: number; }[T]");
AssertType(obj, "{ [_ in T]: number; }");
AssertType(key, "T");
}

