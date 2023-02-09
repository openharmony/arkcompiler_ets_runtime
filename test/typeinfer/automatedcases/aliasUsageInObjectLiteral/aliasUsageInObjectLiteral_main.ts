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

// === tests/cases/compiler/aliasUsageInObjectLiteral_main.ts ===
declare function AssertType(value:any, type:string):void;
import * as Backbone from "./aliasUsageInObjectLiteral_backbone";
import * as moduleA from "./aliasUsageInObjectLiteral_moduleA";
interface IHasVisualizationModel {
    VisualizationModel: typeof Backbone.Model;
}
let a: { x: typeof moduleA } = { x: moduleA };
AssertType(a, "{ x: typeof moduleA; }");
AssertType(x, "typeof moduleA");
AssertType(moduleA, "typeof moduleA");
AssertType({ x: moduleA }, "{ x: typeof moduleA; }");
AssertType(x, "typeof moduleA");
AssertType(moduleA, "typeof moduleA");

let b: { x: IHasVisualizationModel } = { x: moduleA };
AssertType(b, "{ x: IHasVisualizationModel; }");
AssertType(x, "IHasVisualizationModel");
AssertType({ x: moduleA }, "{ x: typeof moduleA; }");
AssertType(x, "typeof moduleA");
AssertType(moduleA, "typeof moduleA");

let c: { y: { z: IHasVisualizationModel } } = { y: { z: moduleA } };
AssertType(c, "{ y: {    z: IHasVisualizationModel;}; }");
AssertType(y, "{ z: IHasVisualizationModel; }");
AssertType(z, "IHasVisualizationModel");
AssertType({ y: { z: moduleA } }, "{ y: { z: typeof moduleA; }; }");
AssertType(y, "{ z: typeof moduleA; }");
AssertType({ z: moduleA }, "{ z: typeof moduleA; }");
AssertType(z, "typeof moduleA");
AssertType(moduleA, "typeof moduleA");


