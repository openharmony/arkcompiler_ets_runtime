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

// === tests/cases/compiler/augment.ts ===
declare function AssertType(value:any, type:string):void;
import * as ns from "./reexport";

declare module "./reexport" {
    export interface Foo {
        self: Foo;
    }
}

declare const f: ns.Foo;
AssertType(f, "ns.Foo");
AssertType(ns, "any");

f.x;
AssertType(f.x, "number");

f.self;
AssertType(f.self, "ns.Foo");

f.self.x;
AssertType(f.self.x, "number");

f.self.self;
AssertType(f.self.self, "ns.Foo");


