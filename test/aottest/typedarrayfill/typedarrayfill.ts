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

function testUInt8Array() {
    let length = 10000;
    let arr = new Int8Array(length)
    let start = 1234;
    let end = 5678;
    arr.fill(2, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 2) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    if (bk) {
        print("testUInt8Array - success");
    } else {
        print("testUInt8Array - failed");
    }
}

function testUint8ClampedArray() {
    let length = 10000;
    let arr = new Uint8ClampedArray(length)
    let start = 1234;
    let end = 5678;
    arr.fill(2, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 2) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    if (bk) {
        print("testUint8ClampedArray - success");
    } else {
        print("testUint8ClampedArray - failed");
    }
}

function testInt8Array() {
    let length = 10000;
    let arr = new Int8Array(length)
    let start = 1234;
    let end = 5678;
    arr.fill(2, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 2) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    if (bk) {
        print("testInt8Array - success");
    } else {
        print("testInt8Array - failed");
    }
}

function testUint16Array() {
    let length = 10000;
    let arr = new Uint16Array(length)
    let start = 1234;
    let end = 5678;
    arr.fill(2, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 2) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    if (bk) {
        print("testUint16Array - success");
    } else {
        print("testUint16Array - failed");
    }
}

function testInt16Array() {
    let length = 10000;
    let arr = new Int16Array(length)
    let start = 1234;
    let end = 5678;
    arr.fill(2, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 2) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    if (bk) {
        print("testInt16Array - success");
    } else {
        print("testInt16Array - failed");
    }
}

function testUInt32Array() {
    let length = 10000;
    let arr = new Uint32Array(length)
    let start = 1234;
    let end = 5678;
    arr.fill(2, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 2) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    if (bk) {
        print("testUInt32Array - success");
    } else {
        print("testUInt32Array - failed");
    }
}

function testInt32Array() {
    let length = 10000;
    let arr = new Int32Array(length)
    let start = 1234;
    let end = 5678;
    arr.fill(2, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 2) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 2) {
            bk = false;
        }
    }
    if (bk) {
        print("testInt32Array - success");
    } else {
        print("testInt32Array - failed");
    }
}

function testFloat32Array() {
    let length = 10000;
    let arr = new Float32Array(length)
    let start = 1234;
    let end = 5678;
    arr.fill(5.5, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 5.5) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 5.5) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 5.5) {
            bk = false;
        }
    }
    if (bk) {
        print("testFloat32Array - success");
    } else {
        print("testFloat32Array - failed");
    }
}

function testFloat64Array() {
    let length = 10000;
    let arr = new Float64Array(length)
    let start = 1234;
    let end = 5678;
    arr.fill(5.5, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 5.5) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 5.5) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 5.5) {
            bk = false;
        }
    }
    if (bk) {
        print("testFloat64Array - success");
    } else {
        print("testFloat64Array - failed");
    }
}

function testBigInt64Array() {
    let length = 100;
    let arr = new BigInt64Array(length)
    let start = 12;
    let end = 56;
    arr.fill(42n, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 42n) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 42n) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 42n) {
            bk = false;
        }
    }
    if (bk) {
        print("testBigInt64Array - success");
    } else {
        print("testBigInt64Array - failed");
    }
}

function testBigUInt64Array() {
    let length = 100;
    let arr = new BigUint64Array(length)
    let start = 12;
    let end = 56;
    arr.fill(42n, start, end);
    let bk = true;
    for (let i = 0; i < start; i++) {
        if (arr[i] == 42n) {
            bk = false;
        }
    }
    for (let i = start; i < end; i++) {
        if (arr[i] != 42n) {
            bk = false;
        }
    }
    for (let i = end; i < length; i++) {
        if (arr[i] == 42n) {
            bk = false;
        }
    }
    if (bk) {
        print("testBigUInt64Array - success");
    } else {
        print("testBigUInt64Array - failed");
    }
}

testUInt8Array();
testUint8ClampedArray();
testInt8Array();
testUint16Array();
testInt16Array();
testUInt32Array();
testInt32Array();
testFloat32Array();
testFloat64Array();
testBigInt64Array();
testBigUInt64Array();