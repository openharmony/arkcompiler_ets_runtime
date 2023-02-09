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

// === tests/cases/compiler/constructorArgWithGenericCallSignature.ts ===
declare function AssertType(value:any, type:string):void;
module Test {
    export interface MyFunc {
        <T>(value1: T): T;
    }
    export class MyClass {
        constructor(func: MyFunc) { }
    }
 
 export function F(func: MyFunc) { }
}
let func: Test.MyFunc;
AssertType(func, "Test.MyFunc");
AssertType(Test, "any");

Test.F(func); // OK
AssertType(Test.F(func), "void");
AssertType(Test.F, "(Test.MyFunc) => void");
AssertType(func, "Test.MyFunc");

let test = new Test.MyClass(func); // Should be OK
AssertType(test, "Test.MyClass");
AssertType(new Test.MyClass(func), "Test.MyClass");
AssertType(Test.MyClass, "typeof Test.MyClass");
AssertType(func, "Test.MyFunc");


