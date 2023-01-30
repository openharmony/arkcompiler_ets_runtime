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

// === tests/cases/compiler/controlFlowForCompoundAssignmentToThisMember.ts ===
declare function AssertType(value:any, type:string):void;
class DatasourceCommandWidgetElement {
    _commandBased: boolean;
    _commandElement: unknown;
    commandElement: unknown;

    constructor(target: unknown) {
        if (target instanceof DatasourceCommandWidgetElement) {
AssertType(target instanceof DatasourceCommandWidgetElement, "boolean");
AssertType(target, "unknown");
AssertType(DatasourceCommandWidgetElement, "typeof DatasourceCommandWidgetElement");

            this._commandBased = true;
AssertType(this._commandBased = true, "boolean");
AssertType(this._commandBased, "boolean");
AssertType(this, "this");
AssertType(true, "boolean");

            this._commandElement = target.commandElement;
AssertType(this._commandElement = target.commandElement, "unknown");
AssertType(this._commandElement, "unknown");
AssertType(this, "this");
AssertType(target.commandElement, "unknown");

        } else {
            this._commandBased = false;
AssertType(this._commandBased = false, "boolean");
AssertType(this._commandBased, "boolean");
AssertType(this, "this");
AssertType(false, "boolean");
        }

        if (this._commandBased = (target instanceof DatasourceCommandWidgetElement)) {
AssertType(this._commandBased = (target instanceof DatasourceCommandWidgetElement), "boolean");
AssertType(this._commandBased, "boolean");
AssertType(this, "this");
AssertType((target instanceof DatasourceCommandWidgetElement), "boolean");
AssertType(target instanceof DatasourceCommandWidgetElement, "boolean");
AssertType(target, "unknown");
AssertType(DatasourceCommandWidgetElement, "typeof DatasourceCommandWidgetElement");

            this._commandElement = target.commandElement;
AssertType(this._commandElement = target.commandElement, "unknown");
AssertType(this._commandElement, "unknown");
AssertType(this, "this");
AssertType(target.commandElement, "unknown");
        }
    }
}

