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

declare function print(str:any):string;
declare var ArkTools:any;

class A {
    name:string;
    value:number;
    constructor(name:string, value:number) {
        this.name = name;
        this.value = value;
    }
}
class B extends A {
    constructor(name:string, value:number) {
        super(name, value);
    }
}


var obj = new B("AOT", 123);
print(obj.name);
print(obj.value);

// Inconsistent number of parameters
class C extends A {
    constructor(name:string, value:number) {
        super(name, value, value);
    }
}

var obj2 = new C("AOT", 123);
print(ArkTools.isAOTCompiled(C))

class D extends B {
    constructor(name:string, value:number) {
        super(name, value);
    }
}

var obj = new D("AOT", 456);
print(obj.name);
print(obj.value);

function foo () {
    ArkTools.clearTypedOpProfiler();
    B.__proto__ = {}
    try {
        new B("AOT", 123);
    } catch (e) {
        ArkTools.printTypedOpProfiler("INLINE_SUPER_CTOR_CHECK")
        print("not constructor");
    }
}
foo();

class E {
    name:string;
    value:number;
    constructor(name:string, value:number) {
        this.name = name;
        this.value = value;
        this.sin = Math.sin(value)
        return 1
    }
}
class F extends E {
    constructor(name:string, value:number) {
        super(name, value);
    }
}

function bar() {
    let obj1 = new F("AOT", 123);
    Object.defineProperty(Math, 1, {});
    let obj2= new F("AOT", 123);
    print(obj2.name)
    print(obj2.value)
}

bar();