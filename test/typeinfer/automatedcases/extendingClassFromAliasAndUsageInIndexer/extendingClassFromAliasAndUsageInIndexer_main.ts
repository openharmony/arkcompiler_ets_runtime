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

// === tests/cases/compiler/extendingClassFromAliasAndUsageInIndexer_main.ts ===
declare function AssertType(value:any, type:string):void;
import * as Backbone from "./extendingClassFromAliasAndUsageInIndexer_backbone";
import * as moduleA from "./extendingClassFromAliasAndUsageInIndexer_moduleA";
import * as moduleB from "./extendingClassFromAliasAndUsageInIndexer_moduleB";
interface IHasVisualizationModel {
    VisualizationModel: typeof Backbone.Model;
}
let moduleATyped: IHasVisualizationModel = moduleA;
AssertType(moduleATyped, "IHasVisualizationModel");
AssertType(moduleA, "typeof moduleA");

let moduleMap: { [key: string]: IHasVisualizationModel } = {
AssertType(moduleMap, "{ [string]: IHasVisualizationModel; }");
AssertType(key, "string");
AssertType({    "moduleA": moduleA,    "moduleB": moduleB}, "{ moduleA: typeof moduleA; moduleB: typeof moduleB; }");

    "moduleA": moduleA,
AssertType("moduleA", "typeof moduleA");
AssertType(moduleA, "typeof moduleA");

    "moduleB": moduleB
AssertType("moduleB", "typeof moduleB");
AssertType(moduleB, "typeof moduleB");

};
let moduleName: string;
AssertType(moduleName, "string");

let visModel = new moduleMap[moduleName].VisualizationModel();
AssertType(visModel, "Backbone.Model");
AssertType(new moduleMap[moduleName].VisualizationModel(), "Backbone.Model");
AssertType(moduleMap[moduleName].VisualizationModel, "typeof Backbone.Model");


