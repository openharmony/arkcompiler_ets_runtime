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

// === tests/cases/compiler/classAttributeInferenceTemplate.ts ===
declare function AssertType(value:any, type:string):void;
class MyClass {
    property;
    property2;

    constructor() {
        const variable = 'something'
AssertType(variable, "string");
AssertType('something', "string");

        this.property = `foo`; // Correctly inferred as `string`
AssertType(this.property = `foo`, "string");
AssertType(this.property, "string");
AssertType(this, "this");
AssertType(`foo`, "string");

        this.property2 = `foo-${variable}`; // Causes an error
AssertType(this.property2 = `foo-${variable}`, "string");
AssertType(this.property2, "string");
AssertType(this, "this");
AssertType(`foo-${variable}`, "string");
AssertType(variable, "string");

        const localProperty = `foo-${variable}`; // Correctly inferred as `string`
AssertType(localProperty, "string");
AssertType(`foo-${variable}`, "string");
AssertType(variable, "string");
    }
}

