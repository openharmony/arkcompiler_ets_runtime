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

// === tests/cases/compiler/spreadExpressionContextualTypeWithNamespace_1.ts ===
declare function AssertType(value:any, type:string):void;
import * as stuff from "./spreadExpressionContextualTypeWithNamespace_0";

stuff.func;
AssertType(stuff.func, "() => void");

stuff.klass;
AssertType(stuff.klass, "typeof stuff.klass");

stuff.obj;
AssertType(stuff.obj, "{ x: boolean; }");

stuff.exportedDirectly;
AssertType(stuff.exportedDirectly, "() => void");

function getStuff<T>() {
  const thing = { ...stuff };
AssertType(thing, "{ exportedDirectly(): void; func: () => void; klass: typeof stuff.klass; obj: { x: boolean; }; }");
AssertType({ ...stuff }, "{ exportedDirectly(): void; func: () => void; klass: typeof stuff.klass; obj: { x: boolean; }; }");
AssertType(stuff, "typeof stuff");

  thing.func;
AssertType(thing.func, "() => void");

  thing.klass;
AssertType(thing.klass, "typeof stuff.klass");

  thing.obj;
AssertType(thing.obj, "{ x: boolean; }");

  thing.exportedDirectly;
AssertType(thing.exportedDirectly, "() => void");

AssertType(thing, "{ exportedDirectly(): void; func: () => void; klass: typeof stuff.klass; obj: { x: boolean; }; }");
  return thing;
}

getStuff().func;
AssertType(getStuff().func, "() => void");

getStuff().klass;
AssertType(getStuff().klass, "typeof stuff.klass");

getStuff().obj;
AssertType(getStuff().obj, "{ x: boolean; }");

getStuff().exportedDirectly;
AssertType(getStuff().exportedDirectly, "() => void");


