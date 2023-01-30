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

// === tests/cases/compiler/deferredLookupTypeResolution2.ts ===
declare function AssertType(value:any, type:string):void;

// Repro from #17456

type StringContains<S extends string, L extends string> = ({ [K in S]: 'true' } & { [key: string]: 'false'})[L];

type ObjectHasKey<O, L extends string> = StringContains<Extract<keyof O, string>, L>;

type A<T> = ObjectHasKey<T, '0'>;

type B = ObjectHasKey<[string, number], '1'>;  // "true"
type C = ObjectHasKey<[string, number], '2'>;  // "false"
type D = A<[string]>;  // "true"

// Error, "false" not handled
type E<T> = { true: 'true' }[ObjectHasKey<T, '1'>];

type Juxtapose<T> = ({ true: 'otherwise' } & { [k: string]: 'true' })[ObjectHasKey<T, '1'>];

// Error, "otherwise" is missing
type DeepError<T> = { true: 'true' }[Juxtapose<T>];

type DeepOK<T> = { true: 'true', otherwise: 'false' }[Juxtapose<T>];


