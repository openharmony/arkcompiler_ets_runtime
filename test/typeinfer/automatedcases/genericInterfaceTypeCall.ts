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

// === tests/cases/compiler/genericInterfaceTypeCall.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo<T> {
    reject(arg: T): void;
}
let foo: Foo<string>
AssertType(foo, "Foo<string>");
 
interface bar<T> {
    fail(func: (arg: T) => void ): void;
    fail2(func2: { (arg: T): void; }): void;
}
let test: bar<string>;
AssertType(test, "bar<string>");
 
test.fail(arg => foo.reject(arg));
AssertType(test.fail(arg => foo.reject(arg)), "void");
AssertType(test.fail, "((string) => void) => void");
AssertType(arg => foo.reject(arg), "(string) => void");
AssertType(arg, "string");
AssertType(foo.reject(arg), "void");
AssertType(foo.reject, "(string) => void");
AssertType(arg, "string");

test.fail2(arg => foo.reject(arg)); // Error: Supplied parameters do not match any signature of call target
AssertType(test.fail2(arg => foo.reject(arg)), "void");
AssertType(test.fail2, "((string) => void) => void");
AssertType(arg => foo.reject(arg), "(string) => void");
AssertType(arg, "string");
AssertType(foo.reject(arg), "void");
AssertType(foo.reject, "(string) => void");
AssertType(arg, "string");


