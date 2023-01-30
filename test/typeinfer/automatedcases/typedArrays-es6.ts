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

// === tests/cases/compiler/typedArrays-es6.ts ===
declare function AssertType(value:any, type:string):void;
const float32Array = new Float32Array(1);
AssertType(float32Array, "Float32Array");
AssertType(new Float32Array(1), "Float32Array");
AssertType(Float32Array, "Float32ArrayConstructor");
AssertType(1, "int");

[...float32Array];
AssertType([...float32Array], "number[]");
AssertType(...float32Array, "number");
AssertType(float32Array, "Float32Array");

const float64Array = new Float64Array(1);
AssertType(float64Array, "Float64Array");
AssertType(new Float64Array(1), "Float64Array");
AssertType(Float64Array, "Float64ArrayConstructor");
AssertType(1, "int");

[...float64Array];
AssertType([...float64Array], "number[]");
AssertType(...float64Array, "number");
AssertType(float64Array, "Float64Array");

const int16Array = new Int16Array(1);
AssertType(int16Array, "Int16Array");
AssertType(new Int16Array(1), "Int16Array");
AssertType(Int16Array, "Int16ArrayConstructor");
AssertType(1, "int");

[...int16Array];
AssertType([...int16Array], "number[]");
AssertType(...int16Array, "number");
AssertType(int16Array, "Int16Array");

const int32Array = new Int32Array(1);
AssertType(int32Array, "Int32Array");
AssertType(new Int32Array(1), "Int32Array");
AssertType(Int32Array, "Int32ArrayConstructor");
AssertType(1, "int");

[...int32Array];
AssertType([...int32Array], "number[]");
AssertType(...int32Array, "number");
AssertType(int32Array, "Int32Array");

const int8Array = new Int8Array(1);
AssertType(int8Array, "Int8Array");
AssertType(new Int8Array(1), "Int8Array");
AssertType(Int8Array, "Int8ArrayConstructor");
AssertType(1, "int");

[...int8Array];
AssertType([...int8Array], "number[]");
AssertType(...int8Array, "number");
AssertType(int8Array, "Int8Array");

const nodeList = new NodeList();
AssertType(nodeList, "NodeList");
AssertType(new NodeList(), "NodeList");
AssertType(NodeList, "{ new (): NodeList; prototype: NodeList; }");

[...nodeList];
AssertType([...nodeList], "Node[]");
AssertType(...nodeList, "Node");
AssertType(nodeList, "NodeList");

const uint16Array = new Uint16Array(1);
AssertType(uint16Array, "Uint16Array");
AssertType(new Uint16Array(1), "Uint16Array");
AssertType(Uint16Array, "Uint16ArrayConstructor");
AssertType(1, "int");

[...uint16Array];
AssertType([...uint16Array], "number[]");
AssertType(...uint16Array, "number");
AssertType(uint16Array, "Uint16Array");

const uint32Array = new Uint32Array(1);
AssertType(uint32Array, "Uint32Array");
AssertType(new Uint32Array(1), "Uint32Array");
AssertType(Uint32Array, "Uint32ArrayConstructor");
AssertType(1, "int");

[...uint32Array];
AssertType([...uint32Array], "number[]");
AssertType(...uint32Array, "number");
AssertType(uint32Array, "Uint32Array");

const uint8Array = new Uint8Array(1);
AssertType(uint8Array, "Uint8Array");
AssertType(new Uint8Array(1), "Uint8Array");
AssertType(Uint8Array, "Uint8ArrayConstructor");
AssertType(1, "int");

[...uint8Array];
AssertType([...uint8Array], "number[]");
AssertType(...uint8Array, "number");
AssertType(uint8Array, "Uint8Array");

const uint8ClampedArray = new Uint8ClampedArray(1);
AssertType(uint8ClampedArray, "Uint8ClampedArray");
AssertType(new Uint8ClampedArray(1), "Uint8ClampedArray");
AssertType(Uint8ClampedArray, "Uint8ClampedArrayConstructor");
AssertType(1, "int");

[...uint8ClampedArray];
AssertType([...uint8ClampedArray], "number[]");
AssertType(...uint8ClampedArray, "number");
AssertType(uint8ClampedArray, "Uint8ClampedArray");


