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

function testBoolean() {
    return new Boolean();
}
function testNumber() {
    return new Number();
}
function testProxy() {
    const handler = {};
    return new Proxy({}, handler);
}
function testDate() {
    return new Date();
}
function testArray() {
    return new Array();
}
function testSet() {
    return new Set();
}
function testMap() {
    return new Map();
}
function testObject() {
    return new Object();
}
function testError() {
    return new Error();
}
function testInt8Array() {
    return new Int8Array();
}
function testUint8Array() {
    return new Uint8Array();
}
function testUint8ClampedArray() {
    return new Uint8ClampedArray();
}
function testInt16Array() {
    return new Int16Array();
}
function testUint16Array() {
    return new Uint16Array();
}
function testInt32Array() {
    return new Int32Array();
}
function testUint32Array() {
    return new Uint32Array();
}
function testFloat32Array() {
    return new Float32Array();
}
function testFloat64Array() {
    return new Float64Array();
}
function testBigInt64Array() {
    return new BigInt64Array();
}
function testBigUint64Array() {
    return new BigUint64Array();
}

testBoolean();
testNumber();
testProxy();
testDate();
testArray();
testSet();
testMap();
testObject();
testError();
testInt8Array();
testUint8Array();
testUint8ClampedArray();
testInt16Array();
testUint16Array();
testInt32Array();
testUint32Array();
testFloat32Array();
testFloat64Array();
testBigInt64Array();
testBigUint64Array();


ArkTools.arkSteedCompileAsync(testBoolean);
ArkTools.arkSteedCompileAsync(testNumber);
ArkTools.arkSteedCompileAsync(testProxy);
ArkTools.arkSteedCompileAsync(testDate);
ArkTools.arkSteedCompileAsync(testArray);
ArkTools.arkSteedCompileAsync(testSet);
ArkTools.arkSteedCompileAsync(testMap);
ArkTools.arkSteedCompileAsync(testObject);
ArkTools.arkSteedCompileAsync(testError);
ArkTools.arkSteedCompileAsync(testInt8Array);
ArkTools.arkSteedCompileAsync(testUint8Array);
ArkTools.arkSteedCompileAsync(testUint8ClampedArray);
ArkTools.arkSteedCompileAsync(testInt16Array);
ArkTools.arkSteedCompileAsync(testUint16Array);
ArkTools.arkSteedCompileAsync(testInt32Array);
ArkTools.arkSteedCompileAsync(testUint32Array);
ArkTools.arkSteedCompileAsync(testFloat32Array);
ArkTools.arkSteedCompileAsync(testFloat64Array);
ArkTools.arkSteedCompileAsync(testBigInt64Array);
ArkTools.arkSteedCompileAsync(testBigUint64Array);

print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());
print((() => { let time = Date.now(); for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}; return true; })());

class C {};
Boolean = C;
Number = C;
Proxy = C;
Date = C;
Array = C;
Set = C;
Map = C;
Object = C;
Error = C;
Int8Array = C;
Uint8Array = C;
Uint8ClampedArray = C;
Int16Array = C;
Uint16Array = C;
Int32Array = C;
Uint32Array = C;
Float32Array = C;
Float64Array = C;
BigInt64Array = C;
BigUint64Array = C;

testBoolean();
testNumber();
testProxy();
testDate();
testArray();
testSet();
testMap();
testObject();
testError();
testInt8Array();
testUint8Array();
testUint8ClampedArray();
testInt16Array();
testUint16Array();
testInt32Array();
testUint32Array();
testFloat32Array();
testFloat64Array();
testBigInt64Array();
testBigUint64Array();