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

// === tests/cases/compiler/mappedTypeCircularReferenceInAccessor.ts ===
declare function AssertType(value:any, type:string):void;

interface User {
  firstName: string,
  level: number,
  get bestFriend(): User
  set bestFriend(user: SerializablePartial<User>)
}

type FilteredKeys<T> = { [K in keyof T]: T[K] extends number ? K : T[K] extends string ? K : T[K] extends boolean ? K : never }[keyof T];

type SerializablePartial<T> = {
  [K in FilteredKeys<T>]: T[K]
};


