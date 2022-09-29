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
 * @tc.name:typearray
 * @tc.desc:test TypeArray
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
[
    Float64Array,
    Float32Array,
    Int32Array,
    Int16Array,
    Int8Array,
    Uint32Array,
    Uint16Array,
    Uint8Array,
    Uint8ClampedArray
].forEach(function(ctor, i) {
    if (testTypeArray1(ctor) && testTypeArray2(ctor) &&
        testTypeArray3(ctor) && testTypeArray4(ctor)) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArray5(ctor) ) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

function testTypeArray1(ctor) {
    let result = []
    let obj = new ctor(5);
    result.push(obj[0] == 0);
    result.push(obj[1] == 0);
    obj[0] = "123";
    result.push(obj[0] == 123)
    result.push(obj["a"] == undefined);
    obj["a"] = "abc";
    result.push(obj["a"] == "abc");
    obj["-0"] = 12;
    result.push(obj["-0"] == undefined)
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray2(ctor) {
    let result = []
    let obj = new ctor(5);
    for (let i = 0; i < 5; i++) {
        obj[i] = i;
    }
    let child = Object.create(obj);
    for (let i = 0; i < 5; i++) {
        result.push(child[i] == undefined);
    }
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray3(ctor) {
    let result = []
    let obj = new ctor(5);
    let parent = {5: 5, "a": "a"};
    obj.__proto__ = parent;
    result.push(obj[4] == 0);
    result.push(obj[5] == undefined);
    result.push(obj["a"] == "a");
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray4(ctor) {
    let a1 = new Array(1024);
    let a2 = new ctor(1024);
    a2.set(a1);
    for (let i = 0; i < a2.length; i++) {
        if (a2[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray5(ctor) {
    let result = []
    let a1 = new ctor(10);
    a1.set([21n, 2n, 3n], "2");
    result.push(a1[2] == 21n);
    result.push(a1[3] == 2n);
    result.push(a1[4] == 3n);
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

let a1 = new ArrayBuffer(1024*1024*8);
let a2 = new Uint8Array(a1);
let a3 = Uint8Array.from(a2);
print(a3.length);
