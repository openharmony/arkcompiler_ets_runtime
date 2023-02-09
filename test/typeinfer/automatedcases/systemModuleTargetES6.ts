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

// === tests/cases/compiler/systemModuleTargetES6.ts ===
declare function AssertType(value:any, type:string):void;
export class MyClass { }
export class MyClass2 {
    static value = 42;
    static getInstance() { 
AssertType(MyClass2.value, "number");
return MyClass2.value; 
}
}

export function myFunction() {
AssertType(new MyClass(), "MyClass");
AssertType(MyClass, "typeof MyClass");
    return new MyClass();
}

export function myFunction2() {
AssertType(new MyClass2(), "MyClass2");
AssertType(MyClass2, "typeof MyClass2");
    return new MyClass2();
}

