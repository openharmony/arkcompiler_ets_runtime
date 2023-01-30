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

// === tests/cases/compiler/returnInfiniteIntersection.ts ===
declare function AssertType(value:any, type:string):void;
function recursive() {
    let x = <T>(subkey: T) => recursive();
AssertType(x, "<T>(T) => any & { p: any; }");
AssertType(<T>(subkey: T) => recursive(), "<T>(T) => any & { p: any; }");
AssertType(subkey, "T");
AssertType(recursive(), "(<T>(T) => any & { p: any; }) & { p: any; }");
AssertType(recursive, "() => (<T>(T) => any & { p: any; }) & { p: any; }");

AssertType(x as typeof x & { p }, "(<T>(T) => any & { p: any; }) & { p: any; }");
AssertType(x, "<T>(T) => any & { p: any; }");
AssertType(x, "<T>(T) => any & { p: any; }");
AssertType(p, "any");
    return x as typeof x & { p };
}

let result = recursive()(1)
AssertType(result, "(<T>(T) => any & { p: any; }) & { p: any; }");
AssertType(recursive()(1), "(<T>(T) => any & { p: any; }) & { p: any; }");
AssertType(recursive(), "(<T>(T) => any & { p: any; }) & { p: any; }");
AssertType(recursive, "() => (<T>(T) => any & { p: any; }) & { p: any; }");
AssertType(1, "int");


