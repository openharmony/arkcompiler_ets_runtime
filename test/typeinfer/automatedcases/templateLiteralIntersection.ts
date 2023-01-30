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

// === tests/cases/compiler/templateLiteralIntersection.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/48034
const a = 'a'
AssertType(a, "string");
AssertType('a', "string");

type A = typeof a
type MixA = A & {foo: string}

type OriginA1 = `${A}`
type OriginA2 = `${MixA}`

type B = `${typeof a}`
type MixB = B & { foo: string }

type OriginB1 = `${B}`
type OriginB2 = `${MixB}`

type MixC = { foo: string } & A

type OriginC = `${MixC}`

type MixD<T extends string> =
    `${T & { foo: string }}`
type OriginD = `${MixD<A & { foo: string }> & { foo: string }}`;

type E = `${A & {}}`;
type MixE = E & {}
type OriginE = `${MixE}`

type OriginF = `${A}foo${A}`;

