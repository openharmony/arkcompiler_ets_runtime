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

// === tests/cases/compiler/divergentAccessors1.ts ===
declare function AssertType(value:any, type:string):void;
// Accessors in interfaces/types

{
    interface IHasGetSet {
        get foo(): number;
AssertType(foo, "number");

        set foo(v: number | string);
AssertType(foo, "number");
AssertType(v, "union");
    }
    
    const ihgs: IHasGetSet = null as any;
AssertType(ihgs, "IHasGetSet");
AssertType(null as any, "any");
AssertType(null, "null");

    ihgs.foo = "32";
AssertType(ihgs.foo = "32", "string");
AssertType(ihgs.foo, "union");
AssertType("32", "string");

    let r_ihgs_foo: number = ihgs.foo;
AssertType(r_ihgs_foo, "number");
AssertType(ihgs.foo, "number");
}

{
    type T_HasGetSet = {
AssertType(T_HasGetSet, "{ foo: number; }");

        get foo(): number;
AssertType(foo, "number");

        set foo(v: number | string);
AssertType(foo, "number");
AssertType(v, "union");
    }
    
    const t_hgs: T_HasGetSet = null as any;
AssertType(t_hgs, "{ foo: number; }");
AssertType(null as any, "any");
AssertType(null, "null");

    t_hgs.foo = "32";
AssertType(t_hgs.foo = "32", "string");
AssertType(t_hgs.foo, "union");
AssertType("32", "string");

    let r_t_hgs_foo: number = t_hgs.foo;
AssertType(r_t_hgs_foo, "number");
AssertType(t_hgs.foo, "number");
}


