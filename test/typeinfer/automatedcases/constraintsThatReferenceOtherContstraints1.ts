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

// === tests/cases/compiler/constraintsThatReferenceOtherContstraints1.ts ===
declare function AssertType(value:any, type:string):void;
interface Object { }

class Foo<T, U extends T> { }
class Bar<T extends Object, U extends T> {
    data: Foo<Object, Object>; // Error 1 Type 'Object' does not satisfy the constraint 'T' for type parameter 'U extends T'.
}

let x: Foo< { a: string }, { a: string; b: number }>; // Error 2 Type '{ a: string; b: number; }' does not satisfy the constraint 'T' for type 
AssertType(x, "Foo<{ a: string; }, { a: string; b: number; }>");
AssertType(a, "string");
AssertType(a, "string");
AssertType(b, "number");


