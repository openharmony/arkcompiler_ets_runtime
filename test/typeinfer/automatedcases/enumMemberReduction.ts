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

// === tests/cases/compiler/enumMemberReduction.ts ===
declare function AssertType(value:any, type:string):void;
enum MyEnum {
  A,
  B,
  C,
}

enum MyStringEnum {
  A = "a",
  B = "b",
  C = "c",
}

enum MyStringEnumWithEmpty {
  A = "",
  B = "b",
  C = "c",
}

export function fn(optionalEnum: MyEnum | undefined) {
AssertType(optionalEnum ?? MyEnum.A, "MyEnum");
AssertType(optionalEnum, "MyEnum");
AssertType(MyEnum.A, "MyEnum.A");
  return optionalEnum ?? MyEnum.A;
}

export function fn2(optionalEnum: MyEnum | undefined) {
AssertType(optionalEnum || MyEnum.B, "union");
AssertType(optionalEnum, "MyEnum");
AssertType(MyEnum.B, "MyEnum.B");
  return optionalEnum || MyEnum.B;
}

export function fn3(optionalEnum?: MyEnum) {
AssertType(optionalEnum ?? MyEnum.A, "MyEnum");
AssertType(optionalEnum, "MyEnum");
AssertType(MyEnum.A, "MyEnum.A");
  return optionalEnum ?? MyEnum.A;
}

export function fn4(optionalEnum?: MyEnum) {
AssertType(optionalEnum || MyEnum.B, "union");
AssertType(optionalEnum, "MyEnum");
AssertType(MyEnum.B, "MyEnum.B");
  return optionalEnum || MyEnum.B;
}

export function fn5(optionalEnum?: MyStringEnum) {
AssertType(optionalEnum || MyStringEnum.B, "MyStringEnum");
AssertType(optionalEnum, "MyStringEnum");
AssertType(MyStringEnum.B, "MyStringEnum.B");
  return optionalEnum || MyStringEnum.B;
}

export function fn6(optionalEnum?: MyStringEnumWithEmpty) {
AssertType(optionalEnum || MyStringEnumWithEmpty.B, "union");
AssertType(optionalEnum, "MyStringEnumWithEmpty");
AssertType(MyStringEnumWithEmpty.B, "MyStringEnumWithEmpty.B");
  return optionalEnum || MyStringEnumWithEmpty.B;
}


