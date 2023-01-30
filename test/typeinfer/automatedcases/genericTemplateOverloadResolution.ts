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

// === tests/cases/compiler/genericTemplateOverloadResolution.ts ===
declare function AssertType(value:any, type:string):void;
interface IFooFn {
    (strings: TemplateStringsArray): Promise<{}>;
    <T>(strings: TemplateStringsArray): Promise<T>;
}

declare const fooFn: IFooFn;
AssertType(fooFn, "IFooFn");

declare function expect(x: Promise<number>): void;

expect(fooFn<number>``);
AssertType(expect(fooFn<number>``), "void");
AssertType(expect, "(Promise<number>) => void");
AssertType(fooFn<number>``, "Promise<number>");
AssertType(fooFn, "IFooFn");
AssertType(``, "string");


