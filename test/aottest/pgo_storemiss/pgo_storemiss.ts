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
//for Store Accessor ic miss, shouldn't update op
class C {
    constructor() {
        this.x = 1;
        this.y = 1;
        this.z = 1;
        this.w = 1;
        this.t = 1;
    }
 
    set u(value) {
        Object.defineProperty(this, "u", {
            set () {}
        })
    }
}
 
function foo(obj) {
    obj.u = 123;
}
 
let c1 = new C();
let c2 = new C();
foo(c1);
foo(c2);
print("test accessor ic successful1!");

//for Store Accessor ic miss, if change to DictionaryMode, SetAsMega
const v1 = new BigInt64Array();
const o5 = {
    set d(a4) {
        Object.defineProperty(this, 2, {writable: true, value: 5});
    },
};
o5.d = v1;
print("test accessor ic successful2!");

// ic hint wrong object
var o3 = {
    set d(a4) {
        print("set proto")
        Object.defineProperty(o2,"d",{
            writable: true,
            value:1
        })
    },
    x:123
};

var o2 = {
    __proto__:o3,
    y:234
}
var o1 = {
    __proto__:o2,
    z:345
}
for(let i=0;i<5;i++){
    o1.d = v1;
}