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

// === tests/cases/compiler/optionalAccessorsInInterface1.ts ===
declare function AssertType(value:any, type:string):void;
interface MyPropertyDescriptor {
    get? (): any;
    set? (v: any): void;
}

declare function defineMyProperty(o: any, p: string, attributes: MyPropertyDescriptor): any;
defineMyProperty({}, "name", { get: function () { 
AssertType(defineMyProperty({}, "name", { get: function () { return 5; } }), "any");
AssertType(defineMyProperty, "(any, string, MyPropertyDescriptor) => any");
AssertType({}, "{}");
AssertType("name", "string");
AssertType({ get: function () { return 5; } }, "{ get: () => number; }");
AssertType(get, "() => number");
AssertType(function () { return 5; }, "() => number");
AssertType(5, "int");
return 5; } });

interface MyPropertyDescriptor2 {
    get?: () => any;
    set?: (v: any) => void;
}

declare function defineMyProperty2(o: any, p: string, attributes: MyPropertyDescriptor2): any;
defineMyProperty2({}, "name", { get: function () { 
AssertType(defineMyProperty2({}, "name", { get: function () { return 5; } }), "any");
AssertType(defineMyProperty2, "(any, string, MyPropertyDescriptor2) => any");
AssertType({}, "{}");
AssertType("name", "string");
AssertType({ get: function () { return 5; } }, "{ get: () => number; }");
AssertType(get, "() => number");
AssertType(function () { return 5; }, "() => number");
AssertType(5, "int");
return 5; } });


