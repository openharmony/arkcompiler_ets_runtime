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

// === tests/cases/compiler/nestedSelf.ts ===
declare function AssertType(value:any, type:string):void;
module M {
 export class C {
   public n = 42;
   public foo() { [1,2,3].map((x) => { 
AssertType([1,2,3].map((x) => { return this.n * x; }), "number[]");
return this.n * x; })

AssertType([1,2,3].map, "<U>((number, number, number[]) => U, ?any) => U[]");

AssertType([1,2,3], "number[]");

AssertType(1, "int");

AssertType(2, "int");

AssertType(3, "int");

AssertType((x) => { return this.n * x; }, "(number) => number");

AssertType(x, "number");

AssertType(this.n * x, "number");

AssertType(this.n, "number");

AssertType(this, "this");

AssertType(x, "number");
}
 }
}



