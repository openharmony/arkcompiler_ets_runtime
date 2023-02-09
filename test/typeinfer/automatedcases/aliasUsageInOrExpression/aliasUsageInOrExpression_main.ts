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

// === tests/cases/compiler/aliasUsageInOrExpression_main.ts ===
declare function AssertType(value:any, type:string):void;
import * as Backbone from "./aliasUsageInOrExpression_backbone";
import * as moduleA from "./aliasUsageInOrExpression_moduleA";
interface IHasVisualizationModel {
    VisualizationModel: typeof Backbone.Model;
}
let i: IHasVisualizationModel;
AssertType(i, "IHasVisualizationModel");

let d1 = i || moduleA;
AssertType(d1, "IHasVisualizationModel");
AssertType(i || moduleA, "IHasVisualizationModel");
AssertType(i, "IHasVisualizationModel");
AssertType(moduleA, "typeof moduleA");

let d2: IHasVisualizationModel = i || moduleA;
AssertType(d2, "IHasVisualizationModel");
AssertType(i || moduleA, "IHasVisualizationModel");
AssertType(i, "IHasVisualizationModel");
AssertType(moduleA, "typeof moduleA");

let d2: IHasVisualizationModel = moduleA || i;
AssertType(d2, "IHasVisualizationModel");
AssertType(moduleA || i, "IHasVisualizationModel");
AssertType(moduleA, "typeof moduleA");
AssertType(i, "IHasVisualizationModel");

let e: { x: IHasVisualizationModel } = <{ x: IHasVisualizationModel }>null || { x: moduleA };
AssertType(e, "{ x: IHasVisualizationModel; }");
AssertType(x, "IHasVisualizationModel");
AssertType(<{ x: IHasVisualizationModel }>null || { x: moduleA }, "{ x: IHasVisualizationModel; }");
AssertType(<{ x: IHasVisualizationModel }>null, "{ x: IHasVisualizationModel; }");
AssertType(x, "IHasVisualizationModel");
AssertType(null, "null");
AssertType({ x: moduleA }, "{ x: typeof moduleA; }");
AssertType(x, "typeof moduleA");
AssertType(moduleA, "typeof moduleA");

let f: { x: IHasVisualizationModel } = <{ x: IHasVisualizationModel }>null ? { x: moduleA } : null;
AssertType(f, "{ x: IHasVisualizationModel; }");
AssertType(x, "IHasVisualizationModel");
AssertType(<{ x: IHasVisualizationModel }>null ? { x: moduleA } : null, "{ x: typeof moduleA; }");
AssertType(<{ x: IHasVisualizationModel }>null, "{ x: IHasVisualizationModel; }");
AssertType(x, "IHasVisualizationModel");
AssertType(null, "null");
AssertType({ x: moduleA }, "{ x: typeof moduleA; }");
AssertType(x, "typeof moduleA");
AssertType(moduleA, "typeof moduleA");
AssertType(null, "null");


