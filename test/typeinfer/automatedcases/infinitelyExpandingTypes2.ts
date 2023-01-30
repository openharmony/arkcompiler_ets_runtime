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

// === tests/cases/compiler/infinitelyExpandingTypes2.ts ===
declare function AssertType(value:any, type:string):void;
interface Foo<T> {
    x: Foo<Foo<T>>;
} 

interface Bar<T> extends Foo<T> {
    y: string;
}

function f(p: Foo<number>) {
    console.log(p);
AssertType(console.log(p), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(p, "Foo<number>");
}

let v: Bar<number> = null;
AssertType(v, "Bar<number>");
AssertType(null, "null");

f(v); // should not error
AssertType(f(v), "void");
AssertType(f, "(Foo<number>) => void");
AssertType(v, "Bar<number>");


