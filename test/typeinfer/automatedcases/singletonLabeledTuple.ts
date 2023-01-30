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

// === tests/cases/compiler/singletonLabeledTuple.ts ===
declare function AssertType(value:any, type:string):void;

type AliasOptional = [p?: number]

// literal type vs type alias
type Literal = [p?: number] extends [unknown] ? true : false // Expect `Literal` to be `false`
type Alias = AliasOptional extends [unknown] ? true : false     // Expect `Alias` to be `false`

// labeled tuple vs normal tuple
type Labeled = [p?: number] extends [unknown] ? true : false   // Expect `Labeled` to be `false`
type Normal = [number?] extends [unknown] ? true : false       // Expect `Normal` to be `false`


type AliasRest = [...p: number[]];

type LiteralRest = [...p: number[]] extends [unknown] ? true : false; // Expect `LiteralRest` to be `false`
type AliasedRest = AliasRest extends [unknown] ? true : false; // Expect `AliasedRest` to be `false`
type NormalRest = [...number[]] extends [unknown] ? true : false; // Expect `NormalRest` to be `false`

