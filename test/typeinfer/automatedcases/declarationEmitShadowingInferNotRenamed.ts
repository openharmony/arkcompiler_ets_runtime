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

// === tests/cases/compiler/declarationEmitShadowingInferNotRenamed.ts ===
declare function AssertType(value:any, type:string):void;
// Any instance type
type Client = string

// Modified instance
type UpdatedClient<C> = C & {foo: number}

export const createClient = <
AssertType(createClient, "<D extends union>(D) => D extends new (...any[]) => infer C ? UpdatedClient<C> : { [K in keyof D]: D[K] extends new (...any[]) => infer C ? UpdatedClient<C> : never; }");
AssertType(<  D extends    | (new (...args: any[]) => Client) // accept class    | Record<string, new (...args: any[]) => Client> // or map of classes>(  clientDef: D): D extends new (...args: any[]) => infer C  ? UpdatedClient<C> // return instance  : {      [K in keyof D]: D[K] extends new (...args: any[]) => infer C // or map of instances respectively        ? UpdatedClient<C>        : never    } => {  return null as any}, "<D extends union>(D) => D extends new (...any[]) => infer C ? UpdatedClient<C> : { [K in keyof D]: D[K] extends new (...any[]) => infer C ? UpdatedClient<C> : never; }");

  D extends
    | (new (...args: any[]) => Client) // accept class
AssertType(args, "any[]");

    | Record<string, new (...args: any[]) => Client> // or map of classes
AssertType(args, "any[]");

>(
  clientDef: D
AssertType(clientDef, "D");

): D extends new (...args: any[]) => infer C
AssertType(args, "any[]");

  ? UpdatedClient<C> // 
return instance
  : {
      [K in keyof D]: D[K] extends new (...args: any[]) => infer C // or map of instances respectively
AssertType(args, "any[]");

        ? UpdatedClient<C>
        : never
    } => {
AssertType(null as any, "any");
AssertType(null, "null");
  return null as any
}

