/*
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
 */

declare function AssertType(value:any, type:string):void;
let a: number = 1;
let b = a + 1;
AssertType(a, "int");
AssertType(b, "int");
namespace A {
    AssertType(a, "int");
    AssertType(b, "int");
    export let c = a + b;
    export let d = "abc";
    AssertType(d, "string");
}
AssertType(A.c, "int");
AssertType(A.d, "string");
AssertType(a, "int");
AssertType(b, "int");
