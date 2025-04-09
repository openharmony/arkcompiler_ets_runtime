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

class A {
    constructor() {
        this.x = 5
    }
    get getx() {
        return this.x
    }
    get getz() {
        return this.z
    }
}

class B extends A {
    constructor() {
        super()
    }
}

class C extends A {
    constructor() {
        super()
    }
}

class D extends A {
    constructor() {
        super()
        this.y = 3
    }
    get gety() {
        return this.y
    }
}

class E extends A {
    constructor() {
        super();
        this.y = 4
    }
    get gety() {
        return this.y
    }
}
class F {
    constructor() {
        this.x = 1
        this.y = 2
        this.z = 3
    }
    get getx() {
        return this.x
    }
    get gety() {
        return this.y
    }
    get getz() {
        return this.z
    }
}

class G {
    constructor() {
        this.z = 4
        this.y = 5
        this.x = 6
    }
    get getz() {
        return this.z
    }
    get gety() {
        return this.y
    }
    get getx() {
        return this.x
    }
}

class H extends A {
    constructor() {
        super();
        this.z = 2
    }
}

class I extends H {
}

class J extends I {
}

let a = new A()
let b = new B()
let c = new C()
let d = new D()
let e = new E()
let f = new F()
let g = new G()
let h = new H()
let i = new I()
let j = new J()

function test1(obj) {
    print(obj.y)
}

function test2(obj) {
    print(obj.y)
}

function test3(obj) {
    print(obj.x)
}

function test4(obj) {
    print(obj.z)
}

function test5(obj) {
    print(obj.x)
}

function testGet1(obj) {
    print(obj.gety)
}

function testGet2(obj) {
    print(obj.gety)
}

function testGet3(obj) {
    print(obj.getx)
}

function testGet4(obj) {
    print(obj.getz)
}

ArkTools.clearTypedOpProfiler();

test1(f)
test1(g)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();

test2(d)
test2(e)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();

test3(b)
test3(c)
test3(d)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();

test4(i)
test4(j)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();

//b and c can be merged but not MONO
test5(b)
test5(c)
test5(g)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();

//optimization not allowed
testGet1(f)
testGet1(g)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();


//optimization not allowed
testGet2(d)
testGet2(e)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();

testGet3(b)
testGet3(c)
testGet3(d)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();

testGet4(i)
testGet4(j)
ArkTools.printTypedOpProfiler("OBJECT_TYPE_CHECK");
ArkTools.clearTypedOpProfiler();