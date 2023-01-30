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

// === tests/cases/compiler/contextualSignatureInstantiation4.ts ===
declare function AssertType(value:any, type:string):void;
// Repros from #32976

declare class Banana<T extends string> { constructor(a: string, property: T) }

declare function fruitFactory1<TFruit>(Fruit: new (...args: any[]) => TFruit): TFruit
const banana1 = fruitFactory1(Banana) // Banana<any>
AssertType(banana1, "Banana<any>");
AssertType(fruitFactory1(Banana), "Banana<any>");
AssertType(fruitFactory1, "<TFruit>(new (...any[]) => TFruit) => TFruit");
AssertType(Banana, "typeof Banana");

declare function fruitFactory2<TFruit>(Fruit: new (a: string, ...args: any[]) => TFruit): TFruit
const banana2 = fruitFactory2(Banana) // Banana<any>
AssertType(banana2, "Banana<any>");
AssertType(fruitFactory2(Banana), "Banana<any>");
AssertType(fruitFactory2, "<TFruit>(new (string, ...any[]) => TFruit) => TFruit");
AssertType(Banana, "typeof Banana");

declare function fruitFactory3<TFruit>(Fruit: new (a: string, s: "foo", ...args: any[]) => TFruit): TFruit
const banana3 = fruitFactory3(Banana) // Banana<"foo">
AssertType(banana3, "Banana<"foo">");
AssertType(fruitFactory3(Banana), "Banana<"foo">");
AssertType(fruitFactory3, "<TFruit>(new (string, "foo", ...any[]) => TFruit) => TFruit");
AssertType(Banana, "typeof Banana");

declare function fruitFactory4<TFruit>(Fruit: new (a: string, ...args: "foo"[]) => TFruit): TFruit
const banana4 = fruitFactory4(Banana) // Banana<"foo">
AssertType(banana4, "Banana<"foo">");
AssertType(fruitFactory4(Banana), "Banana<"foo">");
AssertType(fruitFactory4, "<TFruit>(new (string, ..."foo"[]) => TFruit) => TFruit");
AssertType(Banana, "typeof Banana");

declare function fruitFactory5<TFruit>(Fruit: new (...args: "foo"[]) => TFruit): TFruit
const banana5 = fruitFactory5(Banana) // Banana<"foo">
AssertType(banana5, "Banana<"foo">");
AssertType(fruitFactory5(Banana), "Banana<"foo">");
AssertType(fruitFactory5, "<TFruit>(new (..."foo"[]) => TFruit) => TFruit");
AssertType(Banana, "typeof Banana");


