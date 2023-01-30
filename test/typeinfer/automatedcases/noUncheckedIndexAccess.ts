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

// === tests/cases/compiler/noUncheckedIndexAccess.ts ===
declare function AssertType(value:any, type:string):void;
enum Meat {
    Sausage,
    Bacon
  }
  const sausage = Meat.Sausage
AssertType(sausage, "Meat.Sausage");
AssertType(Meat.Sausage, "Meat.Sausage");

  const valueSausage = Meat[sausage]
AssertType(valueSausage, "string");
AssertType(Meat[sausage], "string");
AssertType(Meat, "typeof Meat");
AssertType(sausage, "Meat.Sausage");

  const bacon = Meat.Bacon
AssertType(bacon, "Meat.Bacon");
AssertType(Meat.Bacon, "Meat.Bacon");

  const valueBacon = Meat[bacon]
AssertType(valueBacon, "string");
AssertType(Meat[bacon], "string");
AssertType(Meat, "typeof Meat");
AssertType(bacon, "Meat.Bacon");

  const union: Meat.Bacon | Meat.Sausage = Meat.Bacon
AssertType(union, "Meat");
AssertType(Meat, "any");
AssertType(Meat, "any");
AssertType(Meat.Bacon, "Meat.Bacon");

  const valueUnion = Meat[union]
AssertType(valueUnion, "string");
AssertType(Meat[union], "string");
AssertType(Meat, "typeof Meat");
AssertType(union, "Meat.Bacon");

  //Avoiding a false positive
  const value = Meat[0]
AssertType(value, "union");
AssertType(Meat[0], "union");
AssertType(Meat, "typeof Meat");
AssertType(0, "int");

  const valueUndefined = "testing"
AssertType(valueUndefined, "string");
AssertType("testing", "string");

  const value2 = Meat[valueUndefined]
AssertType(value2, "error");
AssertType(Meat[valueUndefined], "error");
AssertType(Meat, "typeof Meat");
AssertType(valueUndefined, "string");

  enum A {
    a, b, c
  }
  enum B {
    x, y, z
  }
  
  const value3 = A[B.x];
AssertType(value3, "union");
AssertType(A[B.x], "union");
AssertType(A, "typeof A");
AssertType(B.x, "B.x");


