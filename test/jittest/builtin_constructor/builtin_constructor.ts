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


ArkTools.jitCompileAsync(testBoolean);
ArkTools.jitCompileAsync(testNumber);
ArkTools.jitCompileAsync(testProxy);
ArkTools.jitCompileAsync(testDate);
ArkTools.jitCompileAsync(testArray);
ArkTools.jitCompileAsync(testSet);
ArkTools.jitCompileAsync(testMap);
ArkTools.jitCompileAsync(testObject);
ArkTools.jitCompileAsync(testError);
ArkTools.jitCompileAsync(testInt8Array);
ArkTools.jitCompileAsync(testUint8Array);
ArkTools.jitCompileAsync(testUint8ClampedArray);
ArkTools.jitCompileAsync(testInt16Array);
ArkTools.jitCompileAsync(testUint16Array);
ArkTools.jitCompileAsync(testInt32Array);
ArkTools.jitCompileAsync(testUint32Array);
ArkTools.jitCompileAsync(testFloat32Array);
ArkTools.jitCompileAsync(testFloat64Array);
ArkTools.jitCompileAsync(testBigInt64Array);
ArkTools.jitCompileAsync(testBigUint64Array);

print(ArkTools.waitJitCompileFinish(testBoolean));
print(ArkTools.waitJitCompileFinish(testNumber));
print(ArkTools.waitJitCompileFinish(testProxy));
print(ArkTools.waitJitCompileFinish(testDate));
print(ArkTools.waitJitCompileFinish(testArray));
print(ArkTools.waitJitCompileFinish(testSet));
print(ArkTools.waitJitCompileFinish(testMap));
print(ArkTools.waitJitCompileFinish(testObject));
print(ArkTools.waitJitCompileFinish(testError));
print(ArkTools.waitJitCompileFinish(testInt8Array));
print(ArkTools.waitJitCompileFinish(testUint8Array));
print(ArkTools.waitJitCompileFinish(testUint8ClampedArray));
print(ArkTools.waitJitCompileFinish(testInt16Array));
print(ArkTools.waitJitCompileFinish(testUint16Array));
print(ArkTools.waitJitCompileFinish(testInt32Array));
print(ArkTools.waitJitCompileFinish(testUint32Array));
print(ArkTools.waitJitCompileFinish(testFloat32Array));
print(ArkTools.waitJitCompileFinish(testFloat64Array));
print(ArkTools.waitJitCompileFinish(testBigInt64Array));
print(ArkTools.waitJitCompileFinish(testBigUint64Array));

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