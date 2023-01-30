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

// === tests/cases/compiler/specializedOverloadWithRestParameters.ts ===
declare function AssertType(value:any, type:string):void;
class Base { foo() { } }
class Derived1 extends Base { bar() { } }
function f(tagName: 'span', ...args): Derived1;   // error
function f(tagName: number, ...args): Base;
function f(tagName: any): Base {
AssertType(null, "null");
    return null;
}
function g(tagName: 'span', arg): Derived1;    // error
function g(tagName: number, arg): Base;
function g(tagName: any): Base {
AssertType(null, "null");
    return null;
}

