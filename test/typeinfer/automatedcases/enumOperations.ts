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

// === tests/cases/compiler/enumOperations.ts ===
declare function AssertType(value:any, type:string):void;
enum Enum { None = 0 }
let enumType: Enum = Enum.None;
AssertType(enumType, "Enum");
AssertType(Enum.None, "Enum");

let numberType: number = 0;
AssertType(numberType, "number");
AssertType(0, "int");

let anyType: any = 0;
AssertType(anyType, "any");
AssertType(0, "int");
 
enumType ^ numberType;
AssertType(enumType ^ numberType, "number");
AssertType(enumType, "Enum");
AssertType(numberType, "number");

numberType ^ anyType;
AssertType(numberType ^ anyType, "number");
AssertType(numberType, "number");
AssertType(anyType, "any");
 
enumType & anyType;
AssertType(enumType & anyType, "number");
AssertType(enumType, "Enum");
AssertType(anyType, "any");

enumType | anyType;
AssertType(enumType | anyType, "number");
AssertType(enumType, "Enum");
AssertType(anyType, "any");

enumType ^ anyType;
AssertType(enumType ^ anyType, "number");
AssertType(enumType, "Enum");
AssertType(anyType, "any");

~anyType;
AssertType(~anyType, "number");
AssertType(anyType, "any");

enumType <<anyType;
AssertType(enumType <<anyType, "number");
AssertType(enumType, "Enum");
AssertType(anyType, "any");

enumType >>anyType;
AssertType(enumType >>anyType, "number");
AssertType(enumType, "Enum");
AssertType(anyType, "any");

enumType >>>anyType;
AssertType(enumType >>>anyType, "number");
AssertType(enumType, "Enum");
AssertType(anyType, "any");


