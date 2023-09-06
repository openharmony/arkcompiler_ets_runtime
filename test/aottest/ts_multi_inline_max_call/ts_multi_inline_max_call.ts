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

declare function print(arg:any):string;

function foo() {
    foo1();
    foo2();
    foo3();
    foo4();
    foo5();
    foo6();
    foo7();
}

function foo1() {
    bar();
}

function foo2() {
    bar();
}

function foo3() {
    bar();
}

function foo4() {
    bar();
}

function foo5() {
    bar()
}

function foo6() {
    bar();
}

function foo7() {
    bar();
}

function bar() {
    print("bar")
}

foo();