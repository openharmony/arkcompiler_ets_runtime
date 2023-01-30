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

// === tests/cases/compiler/inferConditionalConstraintMappedMember.ts ===
declare function AssertType(value:any, type:string):void;

// Return keyof type without string index signature
type KeysWithoutStringIndex<T> =
    { [K in keyof T]: string extends K ? never : K } extends { [_ in keyof T]: infer U }
    ? U
    : never

// Only "foo" | "bar" as expected, [string] index signature removed
type test = KeysWithoutStringIndex<{ [index: string]: string; foo: string; bar: 'baz' }>
// KeysWithoutStringIndex<T> will always be a subset of keyof T, but is reported as unassignable
export type RemoveIdxSgn<T> = Pick<T, KeysWithoutStringIndex<T>>
  // ERROR:
  // Type 'KeysWithoutStringIndex<T>' does not satisfy the constraint 'keyof T'.
  //  Type 'unknown' is not assignable to type 'keyof T'.(2344)

