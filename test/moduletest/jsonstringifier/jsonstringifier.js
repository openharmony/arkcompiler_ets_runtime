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
 * @tc.name:jsonstringifier
 * @tc.desc:test JSON.stringify
 * @tc.type: FUNC
 * @tc.require: issue#I7DFJC
 */

try {
    const v0 = [1, 2, 3, 4]
    function Foo(a) {
        Object.defineProperty(this, "ownKeys", { configurable: true, enumerable: true, value: a });
    }
    const v1 = new Foo("2060681564", v0, 9.53248718923);
    const v2 = new Proxy({}, v1);
    JSON.stringify(v2);
} catch (e) {
    print("test successful");
}

var obj = {
    2147483648: 2289
}
print(JSON.stringify(obj));

const a = new Uint32Array(0x10);
let  b = a.__proto__;
b[1073741823] = {}
print(JSON.stringify(a))

let o = {
    get g() {
        this[1225] |= 4294967295;    
        return 9;    
    },
    "f1":1,
    "f2":1,
    "f3":1,
    "f4":1,
    "f5":1,
    "f6":1,
    "f7":1,
    "f8":1,
}
print(JSON.stringify(o))
let o2 = {
    get g() {
        delete this.f1;
        return 8;    
    },
    "f1":1,
    "f2":1,
}
print(JSON.stringify(o2))