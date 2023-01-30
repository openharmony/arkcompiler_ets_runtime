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

// === tests/cases/compiler/inlinedAliasAssignableToConstraintSameAsAlias.ts ===
declare function AssertType(value:any, type:string):void;
interface RelationFields {
  x: A;
  y: A[];
  z: A[];
}
type Name = keyof RelationFields;
type ShouldA<RF extends RelationFields, N extends Name> = RF[N] extends A[]
  ? RF[N][0]
  : never;

class A {
  x: A;
  y: A[];
  z: A[];

  whereRelated< // Works // Type is same as A1, but is not assignable to type A
    RF extends RelationFields = RelationFields,
    N extends Name = Name,
    A1 extends A = RF[N] extends A[] ? RF[N][0] : never,
    A2 extends A = ShouldA<RF, N>
  >(): number {
AssertType(1, "int");
    return 1;
  }
}


