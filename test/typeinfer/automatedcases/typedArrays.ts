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

// === tests/cases/compiler/typedArrays.ts ===
declare function AssertType(value:any, type:string):void;
function CreateTypedArrayTypes() {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array;
AssertType(typedArrays[0] = Int8Array, "Int8ArrayConstructor");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array, "Int8ArrayConstructor");

    typedArrays[1] = Uint8Array;
AssertType(typedArrays[1] = Uint8Array, "Uint8ArrayConstructor");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array, "Uint8ArrayConstructor");

    typedArrays[2] = Int16Array;
AssertType(typedArrays[2] = Int16Array, "Int16ArrayConstructor");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array, "Int16ArrayConstructor");

    typedArrays[3] = Uint16Array;
AssertType(typedArrays[3] = Uint16Array, "Uint16ArrayConstructor");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array, "Uint16ArrayConstructor");

    typedArrays[4] = Int32Array;
AssertType(typedArrays[4] = Int32Array, "Int32ArrayConstructor");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array, "Int32ArrayConstructor");

    typedArrays[5] = Uint32Array;
AssertType(typedArrays[5] = Uint32Array, "Uint32ArrayConstructor");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array, "Uint32ArrayConstructor");

    typedArrays[6] = Float32Array;
AssertType(typedArrays[6] = Float32Array, "Float32ArrayConstructor");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array, "Float32ArrayConstructor");

    typedArrays[7] = Float64Array;
AssertType(typedArrays[7] = Float64Array, "Float64ArrayConstructor");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array, "Float64ArrayConstructor");

    typedArrays[8] = Uint8ClampedArray;
AssertType(typedArrays[8] = Uint8ClampedArray, "Uint8ClampedArrayConstructor");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray, "Uint8ClampedArrayConstructor");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateTypedArrayInstancesFromLength(obj: number) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = new Int8Array(obj);
AssertType(typedArrays[0] = new Int8Array(obj), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(new Int8Array(obj), "Int8Array");
AssertType(Int8Array, "Int8ArrayConstructor");
AssertType(obj, "number");

    typedArrays[1] = new Uint8Array(obj);
AssertType(typedArrays[1] = new Uint8Array(obj), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(new Uint8Array(obj), "Uint8Array");
AssertType(Uint8Array, "Uint8ArrayConstructor");
AssertType(obj, "number");

    typedArrays[2] = new Int16Array(obj);
AssertType(typedArrays[2] = new Int16Array(obj), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(new Int16Array(obj), "Int16Array");
AssertType(Int16Array, "Int16ArrayConstructor");
AssertType(obj, "number");

    typedArrays[3] = new Uint16Array(obj);
AssertType(typedArrays[3] = new Uint16Array(obj), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(new Uint16Array(obj), "Uint16Array");
AssertType(Uint16Array, "Uint16ArrayConstructor");
AssertType(obj, "number");

    typedArrays[4] = new Int32Array(obj);
AssertType(typedArrays[4] = new Int32Array(obj), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(new Int32Array(obj), "Int32Array");
AssertType(Int32Array, "Int32ArrayConstructor");
AssertType(obj, "number");

    typedArrays[5] = new Uint32Array(obj);
AssertType(typedArrays[5] = new Uint32Array(obj), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(new Uint32Array(obj), "Uint32Array");
AssertType(Uint32Array, "Uint32ArrayConstructor");
AssertType(obj, "number");

    typedArrays[6] = new Float32Array(obj);
AssertType(typedArrays[6] = new Float32Array(obj), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(new Float32Array(obj), "Float32Array");
AssertType(Float32Array, "Float32ArrayConstructor");
AssertType(obj, "number");

    typedArrays[7] = new Float64Array(obj);
AssertType(typedArrays[7] = new Float64Array(obj), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(new Float64Array(obj), "Float64Array");
AssertType(Float64Array, "Float64ArrayConstructor");
AssertType(obj, "number");

    typedArrays[8] = new Uint8ClampedArray(obj);
AssertType(typedArrays[8] = new Uint8ClampedArray(obj), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(new Uint8ClampedArray(obj), "Uint8ClampedArray");
AssertType(Uint8ClampedArray, "Uint8ClampedArrayConstructor");
AssertType(obj, "number");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateTypedArrayInstancesFromArray(obj: number[]) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = new Int8Array(obj);
AssertType(typedArrays[0] = new Int8Array(obj), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(new Int8Array(obj), "Int8Array");
AssertType(Int8Array, "Int8ArrayConstructor");
AssertType(obj, "number[]");

    typedArrays[1] = new Uint8Array(obj);
AssertType(typedArrays[1] = new Uint8Array(obj), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(new Uint8Array(obj), "Uint8Array");
AssertType(Uint8Array, "Uint8ArrayConstructor");
AssertType(obj, "number[]");

    typedArrays[2] = new Int16Array(obj);
AssertType(typedArrays[2] = new Int16Array(obj), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(new Int16Array(obj), "Int16Array");
AssertType(Int16Array, "Int16ArrayConstructor");
AssertType(obj, "number[]");

    typedArrays[3] = new Uint16Array(obj);
AssertType(typedArrays[3] = new Uint16Array(obj), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(new Uint16Array(obj), "Uint16Array");
AssertType(Uint16Array, "Uint16ArrayConstructor");
AssertType(obj, "number[]");

    typedArrays[4] = new Int32Array(obj);
AssertType(typedArrays[4] = new Int32Array(obj), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(new Int32Array(obj), "Int32Array");
AssertType(Int32Array, "Int32ArrayConstructor");
AssertType(obj, "number[]");

    typedArrays[5] = new Uint32Array(obj);
AssertType(typedArrays[5] = new Uint32Array(obj), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(new Uint32Array(obj), "Uint32Array");
AssertType(Uint32Array, "Uint32ArrayConstructor");
AssertType(obj, "number[]");

    typedArrays[6] = new Float32Array(obj);
AssertType(typedArrays[6] = new Float32Array(obj), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(new Float32Array(obj), "Float32Array");
AssertType(Float32Array, "Float32ArrayConstructor");
AssertType(obj, "number[]");

    typedArrays[7] = new Float64Array(obj);
AssertType(typedArrays[7] = new Float64Array(obj), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(new Float64Array(obj), "Float64Array");
AssertType(Float64Array, "Float64ArrayConstructor");
AssertType(obj, "number[]");

    typedArrays[8] = new Uint8ClampedArray(obj);
AssertType(typedArrays[8] = new Uint8ClampedArray(obj), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(new Uint8ClampedArray(obj), "Uint8ClampedArray");
AssertType(Uint8ClampedArray, "Uint8ClampedArrayConstructor");
AssertType(obj, "number[]");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateIntegerTypedArraysFromArray2(obj:number[]) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array.from(obj);
AssertType(typedArrays[0] = Int8Array.from(obj), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array.from(obj), "Int8Array");
AssertType(Int8Array.from, "{ (ArrayLike<number>): Int8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int8Array; (Iterable<number>, ?(number, number) => number, ?any): Int8Array; }");
AssertType(obj, "number[]");

    typedArrays[1] = Uint8Array.from(obj);
AssertType(typedArrays[1] = Uint8Array.from(obj), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array.from(obj), "Uint8Array");
AssertType(Uint8Array.from, "{ (ArrayLike<number>): Uint8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8Array; (Iterable<number>, ?(number, number) => number, ?any): Uint8Array; }");
AssertType(obj, "number[]");

    typedArrays[2] = Int16Array.from(obj);
AssertType(typedArrays[2] = Int16Array.from(obj), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array.from(obj), "Int16Array");
AssertType(Int16Array.from, "{ (ArrayLike<number>): Int16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int16Array; (Iterable<number>, ?(number, number) => number, ?any): Int16Array; }");
AssertType(obj, "number[]");

    typedArrays[3] = Uint16Array.from(obj);
AssertType(typedArrays[3] = Uint16Array.from(obj), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array.from(obj), "Uint16Array");
AssertType(Uint16Array.from, "{ (ArrayLike<number>): Uint16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint16Array; (Iterable<number>, ?(number, number) => number, ?any): Uint16Array; }");
AssertType(obj, "number[]");

    typedArrays[4] = Int32Array.from(obj);
AssertType(typedArrays[4] = Int32Array.from(obj), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array.from(obj), "Int32Array");
AssertType(Int32Array.from, "{ (ArrayLike<number>): Int32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int32Array; (Iterable<number>, ?(number, number) => number, ?any): Int32Array; }");
AssertType(obj, "number[]");

    typedArrays[5] = Uint32Array.from(obj);
AssertType(typedArrays[5] = Uint32Array.from(obj), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array.from(obj), "Uint32Array");
AssertType(Uint32Array.from, "{ (ArrayLike<number>): Uint32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint32Array; (Iterable<number>, ?(number, number) => number, ?any): Uint32Array; }");
AssertType(obj, "number[]");

    typedArrays[6] = Float32Array.from(obj);
AssertType(typedArrays[6] = Float32Array.from(obj), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array.from(obj), "Float32Array");
AssertType(Float32Array.from, "{ (ArrayLike<number>): Float32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float32Array; (Iterable<number>, ?(number, number) => number, ?any): Float32Array; }");
AssertType(obj, "number[]");

    typedArrays[7] = Float64Array.from(obj);
AssertType(typedArrays[7] = Float64Array.from(obj), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array.from(obj), "Float64Array");
AssertType(Float64Array.from, "{ (ArrayLike<number>): Float64Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float64Array; (Iterable<number>, ?(number, number) => number, ?any): Float64Array; }");
AssertType(obj, "number[]");

    typedArrays[8] = Uint8ClampedArray.from(obj);
AssertType(typedArrays[8] = Uint8ClampedArray.from(obj), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray.from(obj), "Uint8ClampedArray");
AssertType(Uint8ClampedArray.from, "{ (ArrayLike<number>): Uint8ClampedArray; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8ClampedArray; (Iterable<number>, ?(number, number) => number, ?any): Uint8ClampedArray; }");
AssertType(obj, "number[]");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateIntegerTypedArraysFromArrayLike(obj:ArrayLike<number>) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array.from(obj);
AssertType(typedArrays[0] = Int8Array.from(obj), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array.from(obj), "Int8Array");
AssertType(Int8Array.from, "{ (ArrayLike<number>): Int8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int8Array; (Iterable<number>, ?(number, number) => number, ?any): Int8Array; }");
AssertType(obj, "ArrayLike<number>");

    typedArrays[1] = Uint8Array.from(obj);
AssertType(typedArrays[1] = Uint8Array.from(obj), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array.from(obj), "Uint8Array");
AssertType(Uint8Array.from, "{ (ArrayLike<number>): Uint8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8Array; (Iterable<number>, ?(number, number) => number, ?any): Uint8Array; }");
AssertType(obj, "ArrayLike<number>");

    typedArrays[2] = Int16Array.from(obj);
AssertType(typedArrays[2] = Int16Array.from(obj), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array.from(obj), "Int16Array");
AssertType(Int16Array.from, "{ (ArrayLike<number>): Int16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int16Array; (Iterable<number>, ?(number, number) => number, ?any): Int16Array; }");
AssertType(obj, "ArrayLike<number>");

    typedArrays[3] = Uint16Array.from(obj);
AssertType(typedArrays[3] = Uint16Array.from(obj), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array.from(obj), "Uint16Array");
AssertType(Uint16Array.from, "{ (ArrayLike<number>): Uint16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint16Array; (Iterable<number>, ?(number, number) => number, ?any): Uint16Array; }");
AssertType(obj, "ArrayLike<number>");

    typedArrays[4] = Int32Array.from(obj);
AssertType(typedArrays[4] = Int32Array.from(obj), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array.from(obj), "Int32Array");
AssertType(Int32Array.from, "{ (ArrayLike<number>): Int32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int32Array; (Iterable<number>, ?(number, number) => number, ?any): Int32Array; }");
AssertType(obj, "ArrayLike<number>");

    typedArrays[5] = Uint32Array.from(obj);
AssertType(typedArrays[5] = Uint32Array.from(obj), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array.from(obj), "Uint32Array");
AssertType(Uint32Array.from, "{ (ArrayLike<number>): Uint32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint32Array; (Iterable<number>, ?(number, number) => number, ?any): Uint32Array; }");
AssertType(obj, "ArrayLike<number>");

    typedArrays[6] = Float32Array.from(obj);
AssertType(typedArrays[6] = Float32Array.from(obj), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array.from(obj), "Float32Array");
AssertType(Float32Array.from, "{ (ArrayLike<number>): Float32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float32Array; (Iterable<number>, ?(number, number) => number, ?any): Float32Array; }");
AssertType(obj, "ArrayLike<number>");

    typedArrays[7] = Float64Array.from(obj);
AssertType(typedArrays[7] = Float64Array.from(obj), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array.from(obj), "Float64Array");
AssertType(Float64Array.from, "{ (ArrayLike<number>): Float64Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float64Array; (Iterable<number>, ?(number, number) => number, ?any): Float64Array; }");
AssertType(obj, "ArrayLike<number>");

    typedArrays[8] = Uint8ClampedArray.from(obj);
AssertType(typedArrays[8] = Uint8ClampedArray.from(obj), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray.from(obj), "Uint8ClampedArray");
AssertType(Uint8ClampedArray.from, "{ (ArrayLike<number>): Uint8ClampedArray; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8ClampedArray; (Iterable<number>, ?(number, number) => number, ?any): Uint8ClampedArray; }");
AssertType(obj, "ArrayLike<number>");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateTypedArraysOf(obj) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array.of(...obj);
AssertType(typedArrays[0] = Int8Array.of(...obj), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array.of(...obj), "Int8Array");
AssertType(Int8Array.of, "(...number[]) => Int8Array");
AssertType(...obj, "any");
AssertType(obj, "any");

    typedArrays[1] = Uint8Array.of(...obj);
AssertType(typedArrays[1] = Uint8Array.of(...obj), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array.of(...obj), "Uint8Array");
AssertType(Uint8Array.of, "(...number[]) => Uint8Array");
AssertType(...obj, "any");
AssertType(obj, "any");

    typedArrays[2] = Int16Array.of(...obj);
AssertType(typedArrays[2] = Int16Array.of(...obj), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array.of(...obj), "Int16Array");
AssertType(Int16Array.of, "(...number[]) => Int16Array");
AssertType(...obj, "any");
AssertType(obj, "any");

    typedArrays[3] = Uint16Array.of(...obj);
AssertType(typedArrays[3] = Uint16Array.of(...obj), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array.of(...obj), "Uint16Array");
AssertType(Uint16Array.of, "(...number[]) => Uint16Array");
AssertType(...obj, "any");
AssertType(obj, "any");

    typedArrays[4] = Int32Array.of(...obj);
AssertType(typedArrays[4] = Int32Array.of(...obj), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array.of(...obj), "Int32Array");
AssertType(Int32Array.of, "(...number[]) => Int32Array");
AssertType(...obj, "any");
AssertType(obj, "any");

    typedArrays[5] = Uint32Array.of(...obj);
AssertType(typedArrays[5] = Uint32Array.of(...obj), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array.of(...obj), "Uint32Array");
AssertType(Uint32Array.of, "(...number[]) => Uint32Array");
AssertType(...obj, "any");
AssertType(obj, "any");

    typedArrays[6] = Float32Array.of(...obj);
AssertType(typedArrays[6] = Float32Array.of(...obj), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array.of(...obj), "Float32Array");
AssertType(Float32Array.of, "(...number[]) => Float32Array");
AssertType(...obj, "any");
AssertType(obj, "any");

    typedArrays[7] = Float64Array.of(...obj);
AssertType(typedArrays[7] = Float64Array.of(...obj), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array.of(...obj), "Float64Array");
AssertType(Float64Array.of, "(...number[]) => Float64Array");
AssertType(...obj, "any");
AssertType(obj, "any");

    typedArrays[8] = Uint8ClampedArray.of(...obj);
AssertType(typedArrays[8] = Uint8ClampedArray.of(...obj), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray.of(...obj), "Uint8ClampedArray");
AssertType(Uint8ClampedArray.of, "(...number[]) => Uint8ClampedArray");
AssertType(...obj, "any");
AssertType(obj, "any");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateTypedArraysOf2() {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array.of(1,2,3,4);
AssertType(typedArrays[0] = Int8Array.of(1,2,3,4), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array.of(1,2,3,4), "Int8Array");
AssertType(Int8Array.of, "(...number[]) => Int8Array");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    typedArrays[1] = Uint8Array.of(1,2,3,4);
AssertType(typedArrays[1] = Uint8Array.of(1,2,3,4), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array.of(1,2,3,4), "Uint8Array");
AssertType(Uint8Array.of, "(...number[]) => Uint8Array");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    typedArrays[2] = Int16Array.of(1,2,3,4);
AssertType(typedArrays[2] = Int16Array.of(1,2,3,4), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array.of(1,2,3,4), "Int16Array");
AssertType(Int16Array.of, "(...number[]) => Int16Array");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    typedArrays[3] = Uint16Array.of(1,2,3,4);
AssertType(typedArrays[3] = Uint16Array.of(1,2,3,4), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array.of(1,2,3,4), "Uint16Array");
AssertType(Uint16Array.of, "(...number[]) => Uint16Array");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    typedArrays[4] = Int32Array.of(1,2,3,4);
AssertType(typedArrays[4] = Int32Array.of(1,2,3,4), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array.of(1,2,3,4), "Int32Array");
AssertType(Int32Array.of, "(...number[]) => Int32Array");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    typedArrays[5] = Uint32Array.of(1,2,3,4);
AssertType(typedArrays[5] = Uint32Array.of(1,2,3,4), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array.of(1,2,3,4), "Uint32Array");
AssertType(Uint32Array.of, "(...number[]) => Uint32Array");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    typedArrays[6] = Float32Array.of(1,2,3,4);
AssertType(typedArrays[6] = Float32Array.of(1,2,3,4), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array.of(1,2,3,4), "Float32Array");
AssertType(Float32Array.of, "(...number[]) => Float32Array");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    typedArrays[7] = Float64Array.of(1,2,3,4);
AssertType(typedArrays[7] = Float64Array.of(1,2,3,4), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array.of(1,2,3,4), "Float64Array");
AssertType(Float64Array.of, "(...number[]) => Float64Array");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

    typedArrays[8] = Uint8ClampedArray.of(1,2,3,4);
AssertType(typedArrays[8] = Uint8ClampedArray.of(1,2,3,4), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray.of(1,2,3,4), "Uint8ClampedArray");
AssertType(Uint8ClampedArray.of, "(...number[]) => Uint8ClampedArray");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateTypedArraysFromMapFn2<T>(obj:ArrayLike<T>, mapFn: (n:T, v:number)=> number) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array.from(obj, mapFn);
AssertType(typedArrays[0] = Int8Array.from(obj, mapFn), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array.from(obj, mapFn), "Int8Array");
AssertType(Int8Array.from, "{ (ArrayLike<number>): Int8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int8Array; (Iterable<number>, ?(number, number) => number, ?any): Int8Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

    typedArrays[1] = Uint8Array.from(obj, mapFn);
AssertType(typedArrays[1] = Uint8Array.from(obj, mapFn), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array.from(obj, mapFn), "Uint8Array");
AssertType(Uint8Array.from, "{ (ArrayLike<number>): Uint8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8Array; (Iterable<number>, ?(number, number) => number, ?any): Uint8Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

    typedArrays[2] = Int16Array.from(obj, mapFn);
AssertType(typedArrays[2] = Int16Array.from(obj, mapFn), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array.from(obj, mapFn), "Int16Array");
AssertType(Int16Array.from, "{ (ArrayLike<number>): Int16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int16Array; (Iterable<number>, ?(number, number) => number, ?any): Int16Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

    typedArrays[3] = Uint16Array.from(obj, mapFn);
AssertType(typedArrays[3] = Uint16Array.from(obj, mapFn), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array.from(obj, mapFn), "Uint16Array");
AssertType(Uint16Array.from, "{ (ArrayLike<number>): Uint16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint16Array; (Iterable<number>, ?(number, number) => number, ?any): Uint16Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

    typedArrays[4] = Int32Array.from(obj, mapFn);
AssertType(typedArrays[4] = Int32Array.from(obj, mapFn), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array.from(obj, mapFn), "Int32Array");
AssertType(Int32Array.from, "{ (ArrayLike<number>): Int32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int32Array; (Iterable<number>, ?(number, number) => number, ?any): Int32Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

    typedArrays[5] = Uint32Array.from(obj, mapFn);
AssertType(typedArrays[5] = Uint32Array.from(obj, mapFn), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array.from(obj, mapFn), "Uint32Array");
AssertType(Uint32Array.from, "{ (ArrayLike<number>): Uint32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint32Array; (Iterable<number>, ?(number, number) => number, ?any): Uint32Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

    typedArrays[6] = Float32Array.from(obj, mapFn);
AssertType(typedArrays[6] = Float32Array.from(obj, mapFn), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array.from(obj, mapFn), "Float32Array");
AssertType(Float32Array.from, "{ (ArrayLike<number>): Float32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float32Array; (Iterable<number>, ?(number, number) => number, ?any): Float32Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

    typedArrays[7] = Float64Array.from(obj, mapFn);
AssertType(typedArrays[7] = Float64Array.from(obj, mapFn), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array.from(obj, mapFn), "Float64Array");
AssertType(Float64Array.from, "{ (ArrayLike<number>): Float64Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float64Array; (Iterable<number>, ?(number, number) => number, ?any): Float64Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

    typedArrays[8] = Uint8ClampedArray.from(obj, mapFn);
AssertType(typedArrays[8] = Uint8ClampedArray.from(obj, mapFn), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray.from(obj, mapFn), "Uint8ClampedArray");
AssertType(Uint8ClampedArray.from, "{ (ArrayLike<number>): Uint8ClampedArray; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8ClampedArray; (Iterable<number>, ?(number, number) => number, ?any): Uint8ClampedArray; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateTypedArraysFromMapFn(obj:ArrayLike<number>, mapFn: (n:number, v:number)=> number) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array.from(obj, mapFn);
AssertType(typedArrays[0] = Int8Array.from(obj, mapFn), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array.from(obj, mapFn), "Int8Array");
AssertType(Int8Array.from, "{ (ArrayLike<number>): Int8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int8Array; (Iterable<number>, ?(number, number) => number, ?any): Int8Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

    typedArrays[1] = Uint8Array.from(obj, mapFn);
AssertType(typedArrays[1] = Uint8Array.from(obj, mapFn), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array.from(obj, mapFn), "Uint8Array");
AssertType(Uint8Array.from, "{ (ArrayLike<number>): Uint8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8Array; (Iterable<number>, ?(number, number) => number, ?any): Uint8Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

    typedArrays[2] = Int16Array.from(obj, mapFn);
AssertType(typedArrays[2] = Int16Array.from(obj, mapFn), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array.from(obj, mapFn), "Int16Array");
AssertType(Int16Array.from, "{ (ArrayLike<number>): Int16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int16Array; (Iterable<number>, ?(number, number) => number, ?any): Int16Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

    typedArrays[3] = Uint16Array.from(obj, mapFn);
AssertType(typedArrays[3] = Uint16Array.from(obj, mapFn), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array.from(obj, mapFn), "Uint16Array");
AssertType(Uint16Array.from, "{ (ArrayLike<number>): Uint16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint16Array; (Iterable<number>, ?(number, number) => number, ?any): Uint16Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

    typedArrays[4] = Int32Array.from(obj, mapFn);
AssertType(typedArrays[4] = Int32Array.from(obj, mapFn), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array.from(obj, mapFn), "Int32Array");
AssertType(Int32Array.from, "{ (ArrayLike<number>): Int32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int32Array; (Iterable<number>, ?(number, number) => number, ?any): Int32Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

    typedArrays[5] = Uint32Array.from(obj, mapFn);
AssertType(typedArrays[5] = Uint32Array.from(obj, mapFn), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array.from(obj, mapFn), "Uint32Array");
AssertType(Uint32Array.from, "{ (ArrayLike<number>): Uint32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint32Array; (Iterable<number>, ?(number, number) => number, ?any): Uint32Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

    typedArrays[6] = Float32Array.from(obj, mapFn);
AssertType(typedArrays[6] = Float32Array.from(obj, mapFn), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array.from(obj, mapFn), "Float32Array");
AssertType(Float32Array.from, "{ (ArrayLike<number>): Float32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float32Array; (Iterable<number>, ?(number, number) => number, ?any): Float32Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

    typedArrays[7] = Float64Array.from(obj, mapFn);
AssertType(typedArrays[7] = Float64Array.from(obj, mapFn), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array.from(obj, mapFn), "Float64Array");
AssertType(Float64Array.from, "{ (ArrayLike<number>): Float64Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float64Array; (Iterable<number>, ?(number, number) => number, ?any): Float64Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

    typedArrays[8] = Uint8ClampedArray.from(obj, mapFn);
AssertType(typedArrays[8] = Uint8ClampedArray.from(obj, mapFn), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray.from(obj, mapFn), "Uint8ClampedArray");
AssertType(Uint8ClampedArray.from, "{ (ArrayLike<number>): Uint8ClampedArray; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8ClampedArray; (Iterable<number>, ?(number, number) => number, ?any): Uint8ClampedArray; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateTypedArraysFromThisObj(obj:ArrayLike<number>, mapFn: (n:number, v:number)=> number, thisArg: {}) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[0] = Int8Array.from(obj, mapFn, thisArg), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array.from(obj, mapFn, thisArg), "Int8Array");
AssertType(Int8Array.from, "{ (ArrayLike<number>): Int8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int8Array; (Iterable<number>, ?(number, number) => number, ?any): Int8Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

    typedArrays[1] = Uint8Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[1] = Uint8Array.from(obj, mapFn, thisArg), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array.from(obj, mapFn, thisArg), "Uint8Array");
AssertType(Uint8Array.from, "{ (ArrayLike<number>): Uint8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8Array; (Iterable<number>, ?(number, number) => number, ?any): Uint8Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

    typedArrays[2] = Int16Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[2] = Int16Array.from(obj, mapFn, thisArg), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array.from(obj, mapFn, thisArg), "Int16Array");
AssertType(Int16Array.from, "{ (ArrayLike<number>): Int16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int16Array; (Iterable<number>, ?(number, number) => number, ?any): Int16Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

    typedArrays[3] = Uint16Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[3] = Uint16Array.from(obj, mapFn, thisArg), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array.from(obj, mapFn, thisArg), "Uint16Array");
AssertType(Uint16Array.from, "{ (ArrayLike<number>): Uint16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint16Array; (Iterable<number>, ?(number, number) => number, ?any): Uint16Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

    typedArrays[4] = Int32Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[4] = Int32Array.from(obj, mapFn, thisArg), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array.from(obj, mapFn, thisArg), "Int32Array");
AssertType(Int32Array.from, "{ (ArrayLike<number>): Int32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int32Array; (Iterable<number>, ?(number, number) => number, ?any): Int32Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

    typedArrays[5] = Uint32Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[5] = Uint32Array.from(obj, mapFn, thisArg), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array.from(obj, mapFn, thisArg), "Uint32Array");
AssertType(Uint32Array.from, "{ (ArrayLike<number>): Uint32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint32Array; (Iterable<number>, ?(number, number) => number, ?any): Uint32Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

    typedArrays[6] = Float32Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[6] = Float32Array.from(obj, mapFn, thisArg), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array.from(obj, mapFn, thisArg), "Float32Array");
AssertType(Float32Array.from, "{ (ArrayLike<number>): Float32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float32Array; (Iterable<number>, ?(number, number) => number, ?any): Float32Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

    typedArrays[7] = Float64Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[7] = Float64Array.from(obj, mapFn, thisArg), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array.from(obj, mapFn, thisArg), "Float64Array");
AssertType(Float64Array.from, "{ (ArrayLike<number>): Float64Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float64Array; (Iterable<number>, ?(number, number) => number, ?any): Float64Array; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

    typedArrays[8] = Uint8ClampedArray.from(obj, mapFn, thisArg);
AssertType(typedArrays[8] = Uint8ClampedArray.from(obj, mapFn, thisArg), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray.from(obj, mapFn, thisArg), "Uint8ClampedArray");
AssertType(Uint8ClampedArray.from, "{ (ArrayLike<number>): Uint8ClampedArray; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8ClampedArray; (Iterable<number>, ?(number, number) => number, ?any): Uint8ClampedArray; }");
AssertType(obj, "ArrayLike<number>");
AssertType(mapFn, "(number, number) => number");
AssertType(thisArg, "{}");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

function CreateTypedArraysFromThisObj2<T>(obj:ArrayLike<T>, mapFn: (n:T, v:number)=> number, thisArg: {}) {
    let typedArrays = [];
AssertType(typedArrays, "any[]");
AssertType([], "undefined[]");

    typedArrays[0] = Int8Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[0] = Int8Array.from(obj, mapFn, thisArg), "Int8Array");
AssertType(typedArrays[0], "any");
AssertType(typedArrays, "any[]");
AssertType(0, "int");
AssertType(Int8Array.from(obj, mapFn, thisArg), "Int8Array");
AssertType(Int8Array.from, "{ (ArrayLike<number>): Int8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int8Array; (Iterable<number>, ?(number, number) => number, ?any): Int8Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

    typedArrays[1] = Uint8Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[1] = Uint8Array.from(obj, mapFn, thisArg), "Uint8Array");
AssertType(typedArrays[1], "any");
AssertType(typedArrays, "any[]");
AssertType(1, "int");
AssertType(Uint8Array.from(obj, mapFn, thisArg), "Uint8Array");
AssertType(Uint8Array.from, "{ (ArrayLike<number>): Uint8Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8Array; (Iterable<number>, ?(number, number) => number, ?any): Uint8Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

    typedArrays[2] = Int16Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[2] = Int16Array.from(obj, mapFn, thisArg), "Int16Array");
AssertType(typedArrays[2], "any");
AssertType(typedArrays, "any[]");
AssertType(2, "int");
AssertType(Int16Array.from(obj, mapFn, thisArg), "Int16Array");
AssertType(Int16Array.from, "{ (ArrayLike<number>): Int16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int16Array; (Iterable<number>, ?(number, number) => number, ?any): Int16Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

    typedArrays[3] = Uint16Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[3] = Uint16Array.from(obj, mapFn, thisArg), "Uint16Array");
AssertType(typedArrays[3], "any");
AssertType(typedArrays, "any[]");
AssertType(3, "int");
AssertType(Uint16Array.from(obj, mapFn, thisArg), "Uint16Array");
AssertType(Uint16Array.from, "{ (ArrayLike<number>): Uint16Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint16Array; (Iterable<number>, ?(number, number) => number, ?any): Uint16Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

    typedArrays[4] = Int32Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[4] = Int32Array.from(obj, mapFn, thisArg), "Int32Array");
AssertType(typedArrays[4], "any");
AssertType(typedArrays, "any[]");
AssertType(4, "int");
AssertType(Int32Array.from(obj, mapFn, thisArg), "Int32Array");
AssertType(Int32Array.from, "{ (ArrayLike<number>): Int32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Int32Array; (Iterable<number>, ?(number, number) => number, ?any): Int32Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

    typedArrays[5] = Uint32Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[5] = Uint32Array.from(obj, mapFn, thisArg), "Uint32Array");
AssertType(typedArrays[5], "any");
AssertType(typedArrays, "any[]");
AssertType(5, "int");
AssertType(Uint32Array.from(obj, mapFn, thisArg), "Uint32Array");
AssertType(Uint32Array.from, "{ (ArrayLike<number>): Uint32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint32Array; (Iterable<number>, ?(number, number) => number, ?any): Uint32Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

    typedArrays[6] = Float32Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[6] = Float32Array.from(obj, mapFn, thisArg), "Float32Array");
AssertType(typedArrays[6], "any");
AssertType(typedArrays, "any[]");
AssertType(6, "int");
AssertType(Float32Array.from(obj, mapFn, thisArg), "Float32Array");
AssertType(Float32Array.from, "{ (ArrayLike<number>): Float32Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float32Array; (Iterable<number>, ?(number, number) => number, ?any): Float32Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

    typedArrays[7] = Float64Array.from(obj, mapFn, thisArg);
AssertType(typedArrays[7] = Float64Array.from(obj, mapFn, thisArg), "Float64Array");
AssertType(typedArrays[7], "any");
AssertType(typedArrays, "any[]");
AssertType(7, "int");
AssertType(Float64Array.from(obj, mapFn, thisArg), "Float64Array");
AssertType(Float64Array.from, "{ (ArrayLike<number>): Float64Array; <T>(ArrayLike<T>, (T, number) => number, ?any): Float64Array; (Iterable<number>, ?(number, number) => number, ?any): Float64Array; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

    typedArrays[8] = Uint8ClampedArray.from(obj, mapFn, thisArg);
AssertType(typedArrays[8] = Uint8ClampedArray.from(obj, mapFn, thisArg), "Uint8ClampedArray");
AssertType(typedArrays[8], "any");
AssertType(typedArrays, "any[]");
AssertType(8, "int");
AssertType(Uint8ClampedArray.from(obj, mapFn, thisArg), "Uint8ClampedArray");
AssertType(Uint8ClampedArray.from, "{ (ArrayLike<number>): Uint8ClampedArray; <T>(ArrayLike<T>, (T, number) => number, ?any): Uint8ClampedArray; (Iterable<number>, ?(number, number) => number, ?any): Uint8ClampedArray; }");
AssertType(obj, "ArrayLike<T>");
AssertType(mapFn, "(T, number) => number");
AssertType(thisArg, "{}");

AssertType(typedArrays, "any[]");
    return typedArrays;
}

