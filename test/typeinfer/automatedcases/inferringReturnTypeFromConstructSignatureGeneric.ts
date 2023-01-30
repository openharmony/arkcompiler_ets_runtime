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

// === tests/cases/compiler/inferringReturnTypeFromConstructSignatureGeneric.ts ===
declare function AssertType(value:any, type:string):void;
class GenericObject<T extends {} = {}> {
  give(value: T) {
AssertType(value, "T");
    return value;
  }
}
class GenericNumber<T extends number> {
  give(value: T) {
AssertType(value, "T");
    return value;
  }
}
class GenericNumberOrString<T extends number | string> {
  give(value: T) {
AssertType(value, "T");
    return value;
  }
}

function g<T>(type: new () => T): T {
AssertType(new type(), "T");
AssertType(type, "new () => T");
    return new type();
}

const g1 = g(GenericObject);
AssertType(g1, "GenericObject<{}>");
AssertType(g(GenericObject), "GenericObject<{}>");
AssertType(g, "<T>(new () => T) => T");
AssertType(GenericObject, "typeof GenericObject");

g1.give({});
AssertType(g1.give({}), "{}");
AssertType(g1.give, "({}) => {}");
AssertType({}, "{}");

const g2 = g(GenericNumber);
AssertType(g2, "GenericNumber<number>");
AssertType(g(GenericNumber), "GenericNumber<number>");
AssertType(g, "<T>(new () => T) => T");
AssertType(GenericNumber, "typeof GenericNumber");

g2.give(1);
AssertType(g2.give(1), "number");
AssertType(g2.give, "(number) => number");
AssertType(1, "int");

const g3 = g(GenericNumberOrString);
AssertType(g3, "GenericNumberOrString<union>");
AssertType(g(GenericNumberOrString), "GenericNumberOrString<union>");
AssertType(g, "<T>(new () => T) => T");
AssertType(GenericNumberOrString, "typeof GenericNumberOrString");

g3.give(1);
AssertType(g3.give(1), "union");
AssertType(g3.give, "(union) => union");
AssertType(1, "int");

g3.give('1');
AssertType(g3.give('1'), "union");
AssertType(g3.give, "(union) => union");
AssertType('1', "string");


