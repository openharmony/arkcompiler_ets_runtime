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

// @ts-nocheck
declare function print(arg:any):string;
class A {
    n: number = 2

    x(){}
}

class B {
    m:number = 3

    y(){}
}

async function foo() {
    let a = new A()
    a.x = 1
    let r = await a.n // deopt before suspendgenerator
    print(r)
}

async function bar() {
    let b = new B()
    b.y = 1
    let r = await 1
    let c = b.m // deopt after suspendgenerator
    print(c)
    print(r)
}

foo()
print('-----------------')
bar()
