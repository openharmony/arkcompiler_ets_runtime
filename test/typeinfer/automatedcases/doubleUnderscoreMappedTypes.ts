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

// === tests/cases/compiler/doubleUnderscoreMappedTypes.ts ===
declare function AssertType(value:any, type:string):void;
interface Properties {
    property1: string;
    __property2: string;
}

// As expected, I can make an object satisfying this interface
const ok: Properties = {
AssertType(ok, "Properties");
AssertType({    property1: "",    __property2: ""}, "{ property1: string; __property2: string; }");

    property1: "",
AssertType(property1, "string");
AssertType("", "string");

    __property2: ""
AssertType(__property2, "string");
AssertType("", "string");

};

// As expected, "__property2" is indeed a key of the type
type Keys = keyof Properties;
const k: Keys = "__property2"; // ok
AssertType(k, "keyof Properties");
AssertType("__property2", "string");

// This should be valid
type Property2Type = Properties["__property2"];

// And should work with partial
const partial: Partial<Properties> = {
AssertType(partial, "Partial<Properties>");
AssertType({    property1: "",    __property2: ""}, "{ property1: string; __property2: string; }");

    property1: "",
AssertType(property1, "string");
AssertType("", "string");

    __property2: ""
AssertType(__property2, "string");
AssertType("", "string");

};


