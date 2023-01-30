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

// === tests/cases/compiler/nonNullableAndObjectIntersections.ts ===
declare function AssertType(value:any, type:string):void;

// These should all resolve to never

type T0 = NonNullable<null>;
type T1 = NonNullable<undefined>;
type T2 = null & {};
type T3 = undefined & {};
type T4 = null & undefined;
type T6 = null & { a: string } & {};

// Repro from #50519

type NonNullableNew<T> = T & {};
type NonNullableOld<T> = T extends null | undefined ? never : T;

type TestNew = NonNullableNew<null>;
type TestOld = NonNullableOld<null>;


