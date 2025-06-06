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

class C {};

if(ArkTools.isAOTCompiled(testBoolean)) Boolean = C;
if(ArkTools.isAOTCompiled(testNumber)) Number = C;
if(ArkTools.isAOTCompiled(testProxy)) Proxy = C;
if(ArkTools.isAOTCompiled(testDate)) Date = C;
if(ArkTools.isAOTCompiled(testArray)) Array = C;
if(ArkTools.isAOTCompiled(testSet)) Set = C;
if(ArkTools.isAOTCompiled(testMap)) Map = C;
if(ArkTools.isAOTCompiled(testObject)) Object = C;
if(ArkTools.isAOTCompiled(testError)) Error = C;
if(ArkTools.isAOTCompiled(testInt8Array)) Int8Array = C;
if(ArkTools.isAOTCompiled(testUint8Array)) Uint8Array = C;
if(ArkTools.isAOTCompiled(testUint8ClampedArray)) Uint8ClampedArray = C;
if(ArkTools.isAOTCompiled(testInt16Array)) Int16Array = C;
if(ArkTools.isAOTCompiled(testUint16Array)) Uint16Array = C;
if(ArkTools.isAOTCompiled(testInt32Array)) Int32Array = C;
if(ArkTools.isAOTCompiled(testUint32Array)) Uint32Array = C;
if(ArkTools.isAOTCompiled(testFloat32Array)) Float32Array = C;
if(ArkTools.isAOTCompiled(testFloat64Array)) Float64Array = C;
if(ArkTools.isAOTCompiled(testBigInt64Array)) BigInt64Array = C;
if(ArkTools.isAOTCompiled(testBigUint64Array)) BigUint64Array = C;

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
