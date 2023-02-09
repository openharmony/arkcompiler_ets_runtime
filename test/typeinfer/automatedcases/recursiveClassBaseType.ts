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

// === tests/cases/compiler/recursiveClassBaseType.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #44281

declare const p: <T>(fn: () => T) => T;
AssertType(p, "<T>(() => T) => T");
AssertType(fn, "() => T");

declare const Base: <T>(val: T) => { new(): T };
AssertType(Base, "<T>(T) => new () => T");
AssertType(val, "T");

class C extends Base({ x: p<C[]>(() => []) }) { }

// Repro from #44359

abstract class Base1 {
    abstract root(): Derived1;
}

class Derived1 extends class extends Base1 {
    root() {
AssertType(undefined as any, "any");
AssertType(undefined, "undefined");
        return undefined as any;
    }
}
{ }


