/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
declare function print(arg: any): string;

function foo(a) {
    return a[0];
}


function test() {
    let a = "123456789012345678901234567890";
    let b = "123456789012345678901234567890";
    let c = a + b;
    foo(a);
    foo(c);
}

test();
ArkTools.printTypedOpProfiler("LOAD_ELEMENT");
ArkTools.clearTypedOpProfiler();

let v2 = new SendableMap();
class C3 {
    constructor(a5, a6) {
        ({ "length": a5, } = "-39635");
        print(a5 % -1713856730);
    }
}
const v9 = new C3(SendableMap, '-39635');

function testInc() {
    let x = "filter"["length"]
    x++
    print(x)
}
testInc()
print(ArkTools.isAOTCompiled(testInc))

function testMore() {
    let x = "test1"["length"];
    print(-x)
    print(Math.trunc(x))
    print(x % 22.3)
    print(x + "a")
    print(!x)
    print(x == "6")
    print(x || "6")
    print(x + "6")
    print(+x)
    print(String(x))
    print(Boolean(x))
    print(Number(x))
    let arr = new Int32Array(5).fill(x);
    print(arr[x])
    print(x << 1)
    print(x >>> 1)

    let xx = "test2"["0"];
    print(-xx)
    print(Math.trunc(xx))
    print(xx % 22.3)
    print(xx + "a")
    print(!xx)
    print(xx == "6")
    print(xx || "6")
    print(xx + "6")
    print(+xx)
    print(String(xx))
    print(Boolean(xx))
    print(Number(xx))
    let arrr = new Int32Array(5).fill(xx);
    print(arrr[xx])
    print(xx << 1)
    print(xx >>> 1)

    let xxx = "test3"[0];
    print(-xxx)
    print(Math.trunc(xxx))
    print(xxx % 22.3)
    print(xxx + "a")
    print(!xxx)
    print(xxx == "6")
    print(xxx || "6")
    print(xxx + "6")
    print(+xxx)
    print(String(xxx))
    print(Boolean(xxx))
    print(Number(xxx))
    let arrrr = new Int32Array(5).fill(xxx);
    print(arrrr[xxx])
    print(xxx << 1)
    print(xxx >>> 1)
}

testMore()
print(ArkTools.isAOTCompiled(testMore))

class A {
    constructor() {
        this.x = 1;
        this.y = 2;
    }
}
for(let i = 0; i < 1000; i++) {
    A.prototype["z"] = "z";
}
function fooNew() {
    let a = new A();
    print(a.x);
    print(a.y);
    print(a.z);
}
print(ArkTools.isAOTCompiled(fooNew));
for (let i = 0; i < 2; i++) {
    fooNew();
}

