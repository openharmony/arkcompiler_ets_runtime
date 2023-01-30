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

// === tests/cases/compiler/restParameterTypeInstantiation.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #33823

interface TestGeneric<TG> {
  f: string
  g: TG
}

const removeF = <TX>({ f, ...rest }: TestGeneric<TX>) => {
AssertType(removeF, "<TX>(TestGeneric<TX>) => { g: TX; }");
AssertType(f, "string");
AssertType(rest, "{ g: TX; }");
AssertType(<TX>({ f, ...rest }: TestGeneric<TX>) => {  return rest}, "<TX>(TestGeneric<TX>) => { g: TX; }");

AssertType(rest, "{ g: TX; }");
  return rest
}

const result: number = removeF<number>({ f: '', g: 3 }).g
AssertType(result, "number");
AssertType(removeF<number>({ f: '', g: 3 }).g, "number");


