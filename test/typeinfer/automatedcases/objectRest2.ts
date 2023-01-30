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

// === tests/cases/conformance/types/rest/objectRest2.ts ===
declare function AssertType(value:any, type:string):void;
// test for #12203
declare function connectionFromArray(objects: number, args: any): {};
function rootConnection(name: string) {
AssertType({    resolve: async (context, args) => {        const { objects } = await { objects: 12 };      return {        ...connectionFromArray(objects, args)      };    }  }, "{ resolve: (any, any) => Promise<{}>; }");
  return {

    resolve: async (context, args) => {
AssertType(resolve, "(any, any) => Promise<{}>");
AssertType(context, "any");
AssertType(args, "any");
AssertType(async (context, args) => {        const { objects } = await { objects: 12 };      return {        ...connectionFromArray(objects, args)      };    }, "(any, any) => Promise<{}>");

        const { objects } = await { objects: 12 };
AssertType(objects, "number");
AssertType(await { objects: 12 }, "{ objects: number; }");
AssertType({ objects: 12 }, "{ objects: number; }");
AssertType(objects, "number");
AssertType(12, "int");

AssertType({        ...connectionFromArray(objects, args)      }, "{}");
      return {

        ...connectionFromArray(objects, args)
AssertType(connectionFromArray(objects, args), "{}");
AssertType(connectionFromArray, "(number, any) => {}");
AssertType(objects, "number");
AssertType(args, "any");

      };
    }
  };
}
rootConnection('test');
AssertType(rootConnection('test'), "{ resolve: (any, any) => Promise<{}>; }");
AssertType(rootConnection, "(string) => { resolve: (any, any) => Promise<{}>; }");
AssertType('test', "string");


