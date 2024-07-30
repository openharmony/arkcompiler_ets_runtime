/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

class M {
    constructor(value) {
        this.stageValue = value;
    }
}
class BM extends M {
    constructor(value) {
        super(value);
    }
}

let mdf = new M(2);
print(mdf.stageValue);
for (let i = 0; i < 12; i++) {
    mdf = new BM(9);
}
print(mdf.stageValue);

//deoptimized case:
let args = { name: 'encoding', params: '"utf-8"' }
let isClean = Symbol('isClean')
let my = Symbol('my')
class Base {
    constructor(defaults) {
        this.raws = defaults
        this[isClean] = false
        this[my] = true
    }
}

class Son extends Base {
    constructor(defaults) {
        super(defaults)
        this.type = 'atrule'
    }
}

let b = new Base(args)
print(ArkTools.isAOTCompiled(Base))
print(ArkTools.isAOTDeoptimized(Base))

let s = new Son(args)
print(ArkTools.isAOTCompiled(Son))
print(ArkTools.isAOTDeoptimized(Son))



//optimized case
class Base2 {
    constructor(defaults = {}) {
        this.raws = defaults
    }
}

class Son2 extends Base2 {
    constructor(defaults) {
        super(defaults)
        this.type = 'atrule'
    }
}
let b2 = new Base2(args)
print(ArkTools.isAOTCompiled(Base2))
print(ArkTools.isAOTDeoptimized(Base2))

let s2 = new Son2(args)
print(ArkTools.isAOTCompiled(Son2))
print(ArkTools.isAOTDeoptimized(Son2))