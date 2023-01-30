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

// === tests/cases/compiler/idx.ts ===
declare function AssertType(value:any, type:string):void;
import Foo from "./mod";
import { default as Foo2 } from "./mod";
import { Bar, default as Foo3 } from "./mod";
new Foo();
AssertType(new Foo(), "Foo");
AssertType(Foo, "typeof Foo");

new Foo2();
AssertType(new Foo2(), "Foo");
AssertType(Foo2, "typeof Foo");

new Bar();
AssertType(new Bar(), "Bar");
AssertType(Bar, "typeof Bar");

new Foo3();
AssertType(new Foo3(), "Foo");
AssertType(Foo3, "typeof Foo");


