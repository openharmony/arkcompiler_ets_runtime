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

// === tests/cases/compiler/targetTypeTest2.ts ===
declare function AssertType(value:any, type:string):void;
// Test target typing for array literals and call expressions


let a : any[] = [1,2,"3"];
AssertType(a, "any[]");
AssertType([1,2,"3"], "(union)[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType("3", "string");


function func1(stuff:any[]) { 
AssertType(stuff, "any[]");
return stuff; 
}

function func2(stuff1:string, stuff2:number, stuff3:number) {
AssertType(func1([stuff1, stuff2, stuff3]), "any[]");
AssertType(func1, "(any[]) => any[]");
AssertType([stuff1, stuff2, stuff3], "(union)[]");
AssertType(stuff1, "string");
AssertType(stuff2, "number");
AssertType(stuff3, "number");
	return func1([stuff1, stuff2, stuff3]);
}

