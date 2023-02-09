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

// === tests/cases/compiler/inheritanceOfGenericConstructorMethod1.ts ===
declare function AssertType(value:any, type:string):void;
class A<T> { }
class B<T> extends A<T> {}
let a = new A<Date>();
AssertType(a, "A<Date>");
AssertType(new A<Date>(), "A<Date>");
AssertType(A, "typeof A");

let b1 = new B(); // no error
AssertType(b1, "B<unknown>");
AssertType(new B(), "B<unknown>");
AssertType(B, "typeof B");

let b2: B<Date> = new B<Date>(); // no error
AssertType(b2, "B<Date>");
AssertType(new B<Date>(), "B<Date>");
AssertType(B, "typeof B");

let b3 = new B<Date>(); // error, could not select overload for 'new' expression
AssertType(b3, "B<Date>");
AssertType(new B<Date>(), "B<Date>");
AssertType(B, "typeof B");


