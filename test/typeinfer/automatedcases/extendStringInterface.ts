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

// === tests/cases/conformance/types/primitives/string/extendStringInterface.ts ===
declare function AssertType(value:any, type:string):void;
interface String {
    doStuff(): string;
    doOtherStuff<T>(x:T): T;
}

let x = '';
AssertType(x, "string");
AssertType('', "string");

let a: string = x.doStuff();
AssertType(a, "string");
AssertType(x.doStuff(), "string");
AssertType(x.doStuff, "() => string");

let b: string = x.doOtherStuff('hm');
AssertType(b, "string");
AssertType(x.doOtherStuff('hm'), "string");
AssertType(x.doOtherStuff, "<T>(T) => T");
AssertType('hm', "string");

let c: string = x['doStuff']();
AssertType(c, "string");
AssertType(x['doStuff'](), "string");
AssertType(x['doStuff'], "() => string");
AssertType(x, "string");
AssertType('doStuff', "string");

let d: string = x['doOtherStuff']('hm');
AssertType(d, "string");
AssertType(x['doOtherStuff']('hm'), "string");
AssertType(x['doOtherStuff'], "<T>(T) => T");
AssertType(x, "string");
AssertType('doOtherStuff', "string");
AssertType('hm', "string");


