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

// === tests/cases/compiler/substitutionTypeForIndexedAccessType2.ts ===
declare function AssertType(value:any, type:string):void;

interface Foo {
  foo: string|undefined
}

type Str<T extends string> = T

type Bar<T> = 
  T extends Foo
    ? T['foo'] extends string
      // Type 'T["foo"]' does not satisfy the constraint 'string'.
      //  Type 'string | undefined' is not assignable to type 'string'.
      //   Type 'undefined' is not assignable to type 'string'.(2344)
      ? Str<T['foo']>
      : never
    : never

