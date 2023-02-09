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

// === tests/cases/compiler/ambiguousCallsWhereReturnTypesAgree.ts ===
declare function AssertType(value:any, type:string):void;
class TestClass {
    public bar(x: string): void;
    public bar(x: string[]): void;
    public bar(x: any): void {
        
    }
 
    public foo(x: string): void;
    public foo(x: string[]): void;
    public foo(x: any): void {
        this.bar(x); // should not error
AssertType(this.bar(x), "void");
AssertType(this.bar, "{ (string): void; (string[]): void; }");
AssertType(this, "this");
AssertType(x, "any");
    }
}

class TestClass2 {
    public bar(x: string): number;
    public bar(x: string[]): number;
    public bar(x: any): number {
AssertType(0, "int");
        return 0;
    }
 
    public foo(x: string): number;
    public foo(x: string[]): number;
    public foo(x: any): number {
AssertType(this.bar(x), "number");
AssertType(this.bar, "{ (string): number; (string[]): number; }");
AssertType(this, "this");
AssertType(x, "any");
        return this.bar(x); // should not error
    }
}


