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

// === tests/cases/compiler/propertyNamesWithStringLiteral.ts ===
declare function AssertType(value:any, type:string):void;
class _Color {
    a: number; r: number; g: number; b: number;
}

interface NamedColors {
    azure: _Color;
    "blue": _Color;
    "pale blue": _Color;
}
module Color {
    export let namedColors: NamedColors;
}
let a = Color.namedColors["azure"];
AssertType(a, "_Color");
AssertType(Color.namedColors["azure"], "_Color");
AssertType(Color.namedColors, "NamedColors");
AssertType("azure", "string");

let a = Color.namedColors.blue; // Should not error
AssertType(a, "_Color");
AssertType(Color.namedColors.blue, "_Color");

let a = Color.namedColors["pale blue"]; // should not error
AssertType(a, "_Color");
AssertType(Color.namedColors["pale blue"], "_Color");
AssertType(Color.namedColors, "NamedColors");
AssertType("pale blue", "string");


