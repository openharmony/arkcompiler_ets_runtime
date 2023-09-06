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

function foo1() {
    print("foo1");
}

function foo2() {
    print("foo2");
}

function foo3() {
    print("foo3");
}

function foo4() {
    print("foo4");
}

function foo5() {
    print("foo5");
}

function foo6() {
    print("foo6");
}

function foo7() {
    print("foo7");
}

// max inline call allowed is 5
foo1()
foo2()
foo3()
foo4()
foo5()
foo6()
foo7()