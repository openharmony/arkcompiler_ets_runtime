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

// === tests/cases/compiler/typeParameterFixingWithConstraints.ts ===
declare function AssertType(value:any, type:string):void;
interface IBar {
    [barId: string]: any;
}

interface IFoo {
    foo<TBar extends IBar>(bar: TBar, bar1: (bar: TBar) => TBar, bar2: (bar: TBar) => TBar): TBar;
}

let foo: IFoo;
AssertType(foo, "IFoo");

foo.foo({ bar: null }, bar => null, bar => null);
AssertType(foo.foo({ bar: null }, bar => null, bar => null), "{ bar: any; }");
AssertType(foo.foo, "<TBar extends IBar>(TBar, (TBar) => TBar, (TBar) => TBar) => TBar");
AssertType({ bar: null }, "{ bar: null; }");
AssertType(bar, "null");
AssertType(null, "null");
AssertType(bar => null, "({ bar: any; }) => any");
AssertType(bar, "{ bar: any; }");
AssertType(null, "null");
AssertType(bar => null, "({ bar: any; }) => any");
AssertType(bar, "{ bar: any; }");
AssertType(null, "null");


