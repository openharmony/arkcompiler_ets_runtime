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

class A{}

function foo1 (v:number, s:string):string {
    return v + s;
}

function foo2 (s:string):A {
    return new A();
}

AssertType(foo1, "(number, string) => string");
AssertType(foo1(123, "abc"), "string");

AssertType(foo2, "(string) => A");
AssertType(foo2("abc"), "A");

export {foo1, foo2}
