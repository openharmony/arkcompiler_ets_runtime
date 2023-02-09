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

// === tests/cases/compiler/objectLiteralComputedNameNoDeclarationError.ts ===
declare function AssertType(value:any, type:string):void;
const Foo = {
AssertType(Foo, "{ BANANA: "banana"; }");
AssertType({    BANANA: 'banana' as 'banana',}, "{ BANANA: "banana"; }");

    BANANA: 'banana' as 'banana',
AssertType(BANANA, "string");
AssertType('banana' as 'banana', "string");
AssertType('banana', "string");
}

export const Baa = {
AssertType(Baa, "{ banana: number; }");
AssertType({    [Foo.BANANA]: 1}, "{ banana: number; }");

    [Foo.BANANA]: 1
AssertType([Foo.BANANA], "number");
AssertType(Foo.BANANA, "string");
AssertType(1, "int");

};

