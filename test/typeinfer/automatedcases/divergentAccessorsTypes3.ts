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

// === tests/cases/compiler/divergentAccessorsTypes3.ts ===
declare function AssertType(value:any, type:string):void;
class One {
  get prop1(): string { 
AssertType("", "string");
return ""; 
}

  set prop1(s: string | number) { }

  get prop2(): string { 
AssertType("", "string");
return ""; 
}

  set prop2(s: string | number) { }

  prop3: number;

  get prop4(): string { 
AssertType("", "string");
return ""; 
}

  set prop4(s: string | number) { }
}

class Two {
  get prop1(): string { 
AssertType("", "string");
return ""; 
}

  set prop1(s: string | number) { }

  get prop2(): string { 
AssertType("", "string");
return ""; 
}

  set prop2(s: string) { }

  get prop3(): string { 
AssertType("", "string");
return ""; 
}

  set prop3(s: string | boolean) { }

  get prop4(): string { 
AssertType("", "string");
return ""; 
}

  set prop4(s: string | boolean) { }
}

declare const u1: One|Two;
AssertType(u1, "union");

u1.prop1 = 42;
AssertType(u1.prop1 = 42, "int");
AssertType(u1.prop1, "union");
AssertType(42, "int");

u1.prop1 = "hello";
AssertType(u1.prop1 = "hello", "string");
AssertType(u1.prop1, "union");
AssertType("hello", "string");

u1.prop2 = 42;
AssertType(u1.prop2 = 42, "int");
AssertType(u1.prop2, "union");
AssertType(42, "int");

u1.prop2 = "hello";
AssertType(u1.prop2 = "hello", "string");
AssertType(u1.prop2, "union");
AssertType("hello", "string");

u1.prop3 = 42;
AssertType(u1.prop3 = 42, "int");
AssertType(u1.prop3, "union");
AssertType(42, "int");

u1.prop3 = "hello";
AssertType(u1.prop3 = "hello", "string");
AssertType(u1.prop3, "union");
AssertType("hello", "string");

u1.prop3 = true;
AssertType(u1.prop3 = true, "boolean");
AssertType(u1.prop3, "union");
AssertType(true, "boolean");

u1.prop4 = 42;
AssertType(u1.prop4 = 42, "int");
AssertType(u1.prop4, "union");
AssertType(42, "int");

u1.prop4 = "hello";
AssertType(u1.prop4 = "hello", "string");
AssertType(u1.prop4, "union");
AssertType("hello", "string");

u1.prop4 = true;
AssertType(u1.prop4 = true, "boolean");
AssertType(u1.prop4, "union");
AssertType(true, "boolean");


