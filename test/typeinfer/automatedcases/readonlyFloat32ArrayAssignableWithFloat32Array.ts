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

// === tests/cases/compiler/readonlyFloat32ArrayAssignableWithFloat32Array.ts ===
declare function AssertType(value:any, type:string):void;
function update(b: Readonly<Float32Array>) {
    const c = copy(b);
AssertType(c, "Float32Array");
AssertType(copy(b), "Float32Array");
AssertType(copy, "(Float32Array) => Float32Array");
AssertType(b, "Readonly<Float32Array>");

    add(c, c);
AssertType(add(c, c), "void");
AssertType(add, "(Float32Array, Float32Array, ?Float32Array) => void");
AssertType(c, "Float32Array");
AssertType(c, "Float32Array");
}

function add(a: Float32Array, b: Float32Array, c: Float32Array = a) {
    c[0] = a[0] + b[0];
AssertType(c[0] = a[0] + b[0], "number");
AssertType(c[0], "number");
AssertType(c, "Float32Array");
AssertType(0, "int");
AssertType(a[0] + b[0], "number");
AssertType(a[0], "number");
AssertType(a, "Float32Array");
AssertType(0, "int");
AssertType(b[0], "number");
AssertType(b, "Float32Array");
AssertType(0, "int");
}

function copy(a: Float32Array) {
AssertType(new Float32Array(a), "Float32Array");
AssertType(Float32Array, "Float32ArrayConstructor");
AssertType(a, "Float32Array");
    return new Float32Array(a);
}

