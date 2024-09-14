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

/*
 * @tc.name:sendablefunc
 * @tc.desc:test sendablefunc
 * @tc.type: FUNC
 * @tc.require: issueI8R6LC
 */

// @ts-nocheck
declare function print(str: any): string;

print("========== Test Normal Function ==========")
function fooTop()
{
    'use sendable'
    print("sendable foo top");
}

class C {
    constructor() {
        'use sendable'
    }
    PrintStr() {
        print("sendable class C top");
    }
}
function foo(a : number) {
    'use sendable'
    print("sendable func foo :" + a);
    fooTop();
    let c = new C();
    c.PrintStr();
}

function bar()
{
    'use sendable'
    print("sendable func bar");
}

class A {
    constructor() {
        'use sendable'
    }
    fooA() {
        foo(1);
    }
}

type FuncType = () => void
class B {
    callback: FuncType;
    constructor(f : FuncType) {
        'use sendable'
        this.callback = f;
    }
}
let a = new A()
a.fooA()
let b = new B(bar)
b.callback();

print("========== Test Async Function ==========")
function sendData() {
    'use sendable'
    print("await send data");
}
  
async function fooAsyncTop()
{
    'use sendable'
    print("sendable async foo top");
}

async function fooAsync(a : number) {
    'use sendable'
    print("sendable async func foo :" + a);
    fooAsyncTop();
    let c = new C();
    c.PrintStr();
    await sendData();
}
  
async function barAsync()
{
    'use sendable'
    print("sendable async func bar");
}
  
class D {
    constructor() {
        'use sendable'
    }
    fooD() {
        fooAsync(2);
    }
}
  
type FuncAsyncType = () => Promise<void>

class E {
    callback: FuncAsyncType;
    constructor(f : FuncAsyncType) {
        'use sendable'
        this.callback = f;
    }
}
let d = new D()
d.fooD()
let e = new E(barAsync)
e.callback();
print(fooAsyncTop.name);
print(fooTop.name);
print(fooTop.length)
print(fooTop.prototype);