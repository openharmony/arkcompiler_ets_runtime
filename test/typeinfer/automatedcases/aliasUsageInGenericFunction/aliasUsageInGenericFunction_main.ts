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

// === tests/cases/compiler/aliasUsageInGenericFunction_main.ts ===
declare function AssertType(value:any, type:string):void;
import * as Backbone from "./aliasUsageInGenericFunction_backbone";
import * as moduleA from "./aliasUsageInGenericFunction_moduleA";
interface IHasVisualizationModel {
    VisualizationModel: typeof Backbone.Model;
}
function foo<T extends { a: IHasVisualizationModel }>(x: T) {
AssertType(x, "T");
    return x;
}
let r = foo({ a: moduleA });
AssertType(r, "{ a: typeof moduleA; }");
AssertType(foo({ a: moduleA }), "{ a: typeof moduleA; }");
AssertType(foo, "<T extends { a: IHasVisualizationModel; }>(T) => T");
AssertType({ a: moduleA }, "{ a: typeof moduleA; }");
AssertType(a, "typeof moduleA");
AssertType(moduleA, "typeof moduleA");

let r2 = foo({ a: <IHasVisualizationModel>null });
AssertType(r2, "{ a: IHasVisualizationModel; }");
AssertType(foo({ a: <IHasVisualizationModel>null }), "{ a: IHasVisualizationModel; }");
AssertType(foo, "<T extends { a: IHasVisualizationModel; }>(T) => T");
AssertType({ a: <IHasVisualizationModel>null }, "{ a: IHasVisualizationModel; }");
AssertType(a, "IHasVisualizationModel");
AssertType(<IHasVisualizationModel>null, "IHasVisualizationModel");
AssertType(null, "null");


