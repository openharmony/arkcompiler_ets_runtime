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

// === tests/cases/compiler/selfReferencingTypeReferenceInference.ts ===
declare function AssertType(value:any, type:string):void;

interface Box<T> {
    __: T
}

type Recursive<T> =
    | T
    | Box<Recursive<T>>

type InferRecursive<T> = T extends Recursive<infer R> ? R : "never!"

// the type we are testing with
type t1 = Box<string | Box<number | boolean>>

type t2 = InferRecursive<t1>
type t3 = InferRecursive<Box<string | Box<number | boolean>>> // write t1 explicitly

  // Why is t2 and t3 different??
  // They have same input type!

