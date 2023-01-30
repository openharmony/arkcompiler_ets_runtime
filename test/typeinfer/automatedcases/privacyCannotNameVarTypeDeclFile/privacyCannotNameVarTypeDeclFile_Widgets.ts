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

// === tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets.ts ===
declare function AssertType(value:any, type:string):void;
export class Widget1 {
    name = 'one';
}
export function createWidget1() {
AssertType(new Widget1(), "Widget1");
AssertType(Widget1, "typeof Widget1");
    return new Widget1();
}

export module SpecializedWidget {
    export class Widget2 {
        name = 'one';
    }
    export function createWidget2() {
AssertType(new Widget2(), "Widget2");
AssertType(Widget2, "typeof Widget2");
        return new Widget2();
    }
}


