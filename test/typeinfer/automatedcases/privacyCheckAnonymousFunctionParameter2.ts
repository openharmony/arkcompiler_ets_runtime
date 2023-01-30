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

// === tests/cases/compiler/privacyCheckAnonymousFunctionParameter2.ts ===
declare function AssertType(value:any, type:string):void;
export let x = 1;  // Makes this an external module 
AssertType(x, "number");
AssertType(1, "int");

interface Iterator<T> { x: T }

module Q {
    export function foo<T>(x: (a: Iterator<T>) => number) {
AssertType(x, "(Iterator<T>) => number");
        return x;
    }
}

module Q {
    function bar() {
        foo(null);
AssertType(foo(null), "(Iterator<unknown>) => number");
AssertType(foo, "<T>((Iterator<T>) => number) => (Iterator<T>) => number");
AssertType(null, "null");
    }
}

