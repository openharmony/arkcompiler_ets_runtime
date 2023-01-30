/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/typedArraysSubarray.ts ===
declare function AssertType(value:any, type:string):void;
function int8ArraySubarray() {
    let arr = new Int8Array(10);
AssertType(arr, "Int8Array");
AssertType(new Int8Array(10), "Int8Array");
AssertType(Int8Array, "Int8ArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Int8Array");
AssertType(arr.subarray, "(?number, ?number) => Int8Array");

    arr.subarray(0);
AssertType(arr.subarray(0), "Int8Array");
AssertType(arr.subarray, "(?number, ?number) => Int8Array");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Int8Array");
AssertType(arr.subarray, "(?number, ?number) => Int8Array");
AssertType(0, "int");
AssertType(10, "int");
}

function uint8ArraySubarray() {
    let arr = new Uint8Array(10);
AssertType(arr, "Uint8Array");
AssertType(new Uint8Array(10), "Uint8Array");
AssertType(Uint8Array, "Uint8ArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Uint8Array");
AssertType(arr.subarray, "(?number, ?number) => Uint8Array");

    arr.subarray(0);
AssertType(arr.subarray(0), "Uint8Array");
AssertType(arr.subarray, "(?number, ?number) => Uint8Array");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Uint8Array");
AssertType(arr.subarray, "(?number, ?number) => Uint8Array");
AssertType(0, "int");
AssertType(10, "int");
}

function uint8ClampedArraySubarray() {
    let arr = new Uint8ClampedArray(10);
AssertType(arr, "Uint8ClampedArray");
AssertType(new Uint8ClampedArray(10), "Uint8ClampedArray");
AssertType(Uint8ClampedArray, "Uint8ClampedArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Uint8ClampedArray");
AssertType(arr.subarray, "(?number, ?number) => Uint8ClampedArray");

    arr.subarray(0);
AssertType(arr.subarray(0), "Uint8ClampedArray");
AssertType(arr.subarray, "(?number, ?number) => Uint8ClampedArray");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Uint8ClampedArray");
AssertType(arr.subarray, "(?number, ?number) => Uint8ClampedArray");
AssertType(0, "int");
AssertType(10, "int");
}

function int16ArraySubarray() {
    let arr = new Int16Array(10);
AssertType(arr, "Int16Array");
AssertType(new Int16Array(10), "Int16Array");
AssertType(Int16Array, "Int16ArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Int16Array");
AssertType(arr.subarray, "(?number, ?number) => Int16Array");

    arr.subarray(0);
AssertType(arr.subarray(0), "Int16Array");
AssertType(arr.subarray, "(?number, ?number) => Int16Array");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Int16Array");
AssertType(arr.subarray, "(?number, ?number) => Int16Array");
AssertType(0, "int");
AssertType(10, "int");
}

function uint16ArraySubarray() {
    let arr = new Uint16Array(10);
AssertType(arr, "Uint16Array");
AssertType(new Uint16Array(10), "Uint16Array");
AssertType(Uint16Array, "Uint16ArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Uint16Array");
AssertType(arr.subarray, "(?number, ?number) => Uint16Array");

    arr.subarray(0);
AssertType(arr.subarray(0), "Uint16Array");
AssertType(arr.subarray, "(?number, ?number) => Uint16Array");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Uint16Array");
AssertType(arr.subarray, "(?number, ?number) => Uint16Array");
AssertType(0, "int");
AssertType(10, "int");
}

function int32ArraySubarray() {
    let arr = new Int32Array(10);
AssertType(arr, "Int32Array");
AssertType(new Int32Array(10), "Int32Array");
AssertType(Int32Array, "Int32ArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Int32Array");
AssertType(arr.subarray, "(?number, ?number) => Int32Array");

    arr.subarray(0);
AssertType(arr.subarray(0), "Int32Array");
AssertType(arr.subarray, "(?number, ?number) => Int32Array");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Int32Array");
AssertType(arr.subarray, "(?number, ?number) => Int32Array");
AssertType(0, "int");
AssertType(10, "int");
}

function uint32ArraySubarray() {
    let arr = new Uint32Array(10);
AssertType(arr, "Uint32Array");
AssertType(new Uint32Array(10), "Uint32Array");
AssertType(Uint32Array, "Uint32ArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Uint32Array");
AssertType(arr.subarray, "(?number, ?number) => Uint32Array");

    arr.subarray(0);
AssertType(arr.subarray(0), "Uint32Array");
AssertType(arr.subarray, "(?number, ?number) => Uint32Array");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Uint32Array");
AssertType(arr.subarray, "(?number, ?number) => Uint32Array");
AssertType(0, "int");
AssertType(10, "int");
}

function float32ArraySubarray() {
    let arr = new Float32Array(10);
AssertType(arr, "Float32Array");
AssertType(new Float32Array(10), "Float32Array");
AssertType(Float32Array, "Float32ArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Float32Array");
AssertType(arr.subarray, "(?number, ?number) => Float32Array");

    arr.subarray(0);
AssertType(arr.subarray(0), "Float32Array");
AssertType(arr.subarray, "(?number, ?number) => Float32Array");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Float32Array");
AssertType(arr.subarray, "(?number, ?number) => Float32Array");
AssertType(0, "int");
AssertType(10, "int");
}

function float64ArraySubarray() {
    let arr = new Float64Array(10);
AssertType(arr, "Float64Array");
AssertType(new Float64Array(10), "Float64Array");
AssertType(Float64Array, "Float64ArrayConstructor");
AssertType(10, "int");

    arr.subarray();
AssertType(arr.subarray(), "Float64Array");
AssertType(arr.subarray, "(?number, ?number) => Float64Array");

    arr.subarray(0);
AssertType(arr.subarray(0), "Float64Array");
AssertType(arr.subarray, "(?number, ?number) => Float64Array");
AssertType(0, "int");

    arr.subarray(0, 10);
AssertType(arr.subarray(0, 10), "Float64Array");
AssertType(arr.subarray, "(?number, ?number) => Float64Array");
AssertType(0, "int");
AssertType(10, "int");
}


