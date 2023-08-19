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
        testTypeArray3(ctor) && testTypeArray4(ctor) &&
        testTypeArray6(ctor) && testTypeArray7(ctor)) {
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

function testTypeArray6(ctor) {
    let result = []
    let test = new Array(10);
    for (var i = 0; i < 10; i++) {
        test[i] = i;
    }
    let obj = new ctor(test);
    for (var i = 0; i < 10; i++) {
        result.push(obj[i] == i);
    }
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray7(ctor) {
    let result = []
    let test = new Array(50);
    for (var i = 0; i < 10; i++) {
        test[i] = i;
    }
    let obj = new ctor(test);
    for (var i = 0; i < 10; i++) {
        result.push(obj[i] == i);
    }
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

const a4 = new Uint32Array(1024);
const obj1 = {
    "c": a4,
    __proto__: a4
}
obj1[235] = 1024;
print(obj1[235]);

try {
    const a5 = new Uint8ClampedArray(new ArrayBuffer(1283053413), 9007199254740991);
    a5.copyWithin(-13602);
} catch(e) {
    print("test successful !!!");
}

try {
    const a6 = new BigInt64Array(10);
    Int16Array.apply(null, a6);
} catch(e) {
    print("test successful !!!");
}

const a7 = new BigInt64Array(4);
function foo() {}
const f = new foo();
const protoOf = f.isPrototypeOf;
print(protoOf.apply(protoOf, a7));
const uint8 = new Uint8Array([1, 2, 3]);
const reversedUint8 = uint8.toReversed();
print(reversedUint8); // Uint8Array [3, 2, 1]
print(uint8); // Uint8Array [1, 2, 3]

try {
    const a8 = new Int8Array(new ArrayBuffer(0x40004141, {"maxByteLength": 0x40004141}));
    const a9 = new Float64Array(a8);
} catch (e) {
    print("test successful !!!");
}

try {
    const a10 = [1, 2];
    const a11 = new Uint8Array(a10);
    const a12 = new Uint32Array(a11);
    a12.set(a10, 0x1ffffffff);
} catch (e) {
    print("test successful !!!");
}

try {
    const v17 = new Int16Array(5);
    const v20 = new Int16Array(5);
    v17.set(v20, 4294967295);
} catch (error) {
    print(error)
}

try {
    const v17 = new Int16Array(5);
    const v20 = new Int16Array(5);
    v17.set(v20, 4294967296);
} catch (error) {
    print(error)
}

try {
    new BigUint64Array(new ArrayBuffer(256), 256, 0x1fffffff)
} catch (error) {
    print(error)
}