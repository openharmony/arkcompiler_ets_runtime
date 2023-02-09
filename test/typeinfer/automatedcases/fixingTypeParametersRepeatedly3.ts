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

// === tests/cases/compiler/fixingTypeParametersRepeatedly3.ts ===
declare function AssertType(value:any, type:string):void;
interface Base {
    baseProp;
}
interface Derived extends Base {
    toBase?(): Base;
}

let derived: Derived;
AssertType(derived, "Derived");

declare function foo<T>(x: T, func: (p: T) => T): T;
let result = foo(derived, d => d.toBase());
AssertType(result, "Derived");
AssertType(foo(derived, d => d.toBase()), "Derived");
AssertType(foo, "<T>(T, (T) => T) => T");
AssertType(derived, "Derived");
AssertType(d => d.toBase(), "(Derived) => Base");
AssertType(d, "Derived");
AssertType(d.toBase(), "Base");
AssertType(d.toBase, "() => Base");

// bar should type check just like foo.
// result2 should have the same type as result
declare function bar<T>(x: T, func: (p: T) => T): T;
declare function bar<T>(x: T, func: (p: T) => T): T;
let result2 = bar(derived, d => d.toBase());
AssertType(result2, "Base");
AssertType(bar(derived, d => d.toBase()), "Base");
AssertType(bar, "{ <T>(T, (T) => T): T; <T>(T, (T) => T): T; }");
AssertType(derived, "Derived");
AssertType(d => d.toBase(), "(Derived) => Base");
AssertType(d, "Derived");
AssertType(d.toBase(), "Base");
AssertType(d.toBase, "() => Base");


