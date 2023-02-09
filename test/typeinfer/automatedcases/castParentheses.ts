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

// === tests/cases/compiler/castParentheses.ts ===
declare function AssertType(value:any, type:string):void;
class a {
    static b: any;
}

let b = (<any>a);
AssertType(b, "any");
AssertType((<any>a), "any");
AssertType(<any>a, "any");
AssertType(a, "typeof a");

let b = (<any>a).b;
AssertType(b, "any");
AssertType((<any>a).b, "any");

let b = (<any>a.b).c;
AssertType(b, "any");
AssertType((<any>a.b).c, "any");

let b = (<any>a.b()).c;
AssertType(b, "any");
AssertType((<any>a.b()).c, "any");

let b = (<any>new a);
AssertType(b, "any");
AssertType((<any>new a), "any");
AssertType(<any>new a, "any");
AssertType(new a, "a");
AssertType(a, "typeof a");

let b = (<any>new a.b);
AssertType(b, "any");
AssertType((<any>new a.b), "any");
AssertType(<any>new a.b, "any");
AssertType(new a.b, "any");
AssertType(a.b, "any");

let b = (<any>new a).b 
AssertType(b, "any");
AssertType((<any>new a).b, "any");


