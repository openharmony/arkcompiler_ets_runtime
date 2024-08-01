/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 * @tc.name:builtins
 * @tc.desc:test builtins
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var arrayLong = {"1" : "fds", "wode" : "fff"};
Object.defineProperty(arrayLong, "2", {
    configurable:true,
    enumerable:true,
    value:"ggg",
    writable:true
})
for (const key in arrayLong) {
    print(key);
    delete arrayLong[2];
}

Object.defineProperty(arrayLong, "3", {
    configurable:true,
    enumerable:true,
    value:"ggg",
    writable:true
})
for (const key in arrayLong) {
    print(key);
    Object.defineProperty(arrayLong, "3", {
        configurable:true,
        enumerable:false,
        value:"fff",
        writable:true
    })
}

Object.defineProperty(arrayLong, "4", {
    configurable:true,
    enumerable:false,
    value:"ggg",
    writable:true
})
for (const key in arrayLong) {
    print(key);
    Object.defineProperty(arrayLong, "4", {
        configurable:true,
        enumerable:true,
        value:"fff",
        writable:true
    })
    arrayLong.sss = "fds";
}

const targetObj = {
    _secret: 'easily scared',
    eyeCount: 4
};

Object.defineProperty(targetObj, "wode", {
    configurable:true,
    enumerable:false,
    value:"ggg",
    writable:true
})

const proxy_has = new Proxy(targetObj, {
    has: (target, key) => {
        print("key==",key)
        if (key.includes('_')) {
            return false;
        }
        return key in target;
    }
})

for (const key in proxy_has) {
    print(key);
    delete proxy_has.eyeCount;
}

var view = new Int16Array(3);
view[1] = 23;
view[2] = 45;
view[3] = 67;
view.hhh = "wode";
for (const key in view) {
    print(key);
}
for (const key in view) {
    print(key);
    delete view.hhh;
}

try {
    const obj = {};
    obj.getOwnPropertyDescriptor = {};
    const proxy = new Proxy(new Array(100), obj);
    for (const i in proxy) {}
} catch (err) {
    print(err instanceof TypeError);
}

// PoC testcase
try {
    class C {

    }
    C.getPrototypeOf = 3014;
    const proxy = new Proxy([7], C);
    for (const v in proxy) {
    
    }
} catch (err) {
    print(err instanceof TypeError);
}

// Redefine testcast
Object.prototype.prop0 = 0;
let obj = {prop1:1,prop2:2};
Object.defineProperty(obj,
    'prop0', { enumerable: false, configurable: true, value: 0 });
for (var p in obj)
    print(p);



