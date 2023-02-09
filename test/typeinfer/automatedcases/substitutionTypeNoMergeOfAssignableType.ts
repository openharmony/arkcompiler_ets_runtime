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

// === tests/cases/compiler/substitutionTypeNoMergeOfAssignableType.ts ===
declare function AssertType(value:any, type:string):void;
interface Entry {
    comment?: string;
 }
 
 interface Entity {
     fields: {[key: string]: Entry};
 }
 
 type Fields<E extends Entity> = {
     [P in keyof E["fields"]]: E["fields"][P]
 };
 
 type Nodes<T = any> = {
     [P in keyof T]: T[P] extends Entity
         ? Fields<T[P]>
         : T[P]
 };
 
 function makeEntityStore<T extends Record<string, Entity>>(config: T): Nodes<T> {
AssertType({} as Nodes<T>, "Nodes<T>");
AssertType({}, "{}");
     return {} as Nodes<T>
 }
 
 const myTest = makeEntityStore({ test: { fields: { id: {} } } });
AssertType(myTest, "Nodes<{ test: { fields: { id: {}; }; }; }>");
AssertType(makeEntityStore({ test: { fields: { id: {} } } }), "Nodes<{ test: { fields: { id: {}; }; }; }>");
AssertType(makeEntityStore, "<T extends Record<string, Entity>>(T) => Nodes<T>");
AssertType({ test: { fields: { id: {} } } }, "{ test: { fields: { id: {}; }; }; }");
AssertType(test, "{ fields: { id: {}; }; }");
AssertType({ fields: { id: {} } }, "{ fields: { id: {}; }; }");
AssertType(fields, "{ id: {}; }");
AssertType({ id: {} }, "{ id: {}; }");
AssertType(id, "{}");
AssertType({}, "{}");

 myTest.test
AssertType(myTest.test, "Fields<{ fields: { id: {}; }; }>");
 

