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

// === tests/cases/compiler/narrowingRestGenericCall.ts ===
declare function AssertType(value:any, type:string):void;
interface Slugs {
  foo: string;
  bar: string;
}

function call<T extends object>(obj: T, cb: (val: T) => void) {
  cb(obj);
AssertType(cb(obj), "void");
AssertType(cb, "(T) => void");
AssertType(obj, "T");
}

declare let obj: Slugs;
AssertType(obj, "Slugs");

call(obj, ({foo, ...rest}) => {
AssertType(call(obj, ({foo, ...rest}) => {  console.log(rest.bar);}), "void");
AssertType(call, "<T extends object>(T, (T) => void) => void");
AssertType(obj, "Slugs");
AssertType(({foo, ...rest}) => {  console.log(rest.bar);}, "(Slugs) => void");
AssertType(foo, "string");
AssertType(rest, "{ bar: string; }");

  console.log(rest.bar);
AssertType(console.log(rest.bar), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(rest.bar, "string");

});

