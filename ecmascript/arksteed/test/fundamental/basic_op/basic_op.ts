/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
function basic_op()
{
    // unary op
    let a = 10;
    let b = +a; // unary plus
    let c = -a; // unary minus
    let d = ~a; // bitwise NOT
    let e = !a; // logical NOT

    // binary op
    let f = a + b; // addition
    let g = a - b; // subtraction
    let h = a * b; // multiplication
    let i = a / b; // division
    let j = a % b; // remainder
    let k = a & b; // bitwise AND
    let l = a | b; // bitwise OR
    let m = a ^ b; // bitwise XOR
    let n = a << 1; // left shift
    let o = a >> 1; // right shift
    let p = a >>> 1; // unsigned right shift

    // comparison op (no basic block)
    let cmp1 = a == b;
    let cmp2 = a != b;
    let cmp3 = a === b;
    let cmp4 = a !== b;
    let cmp5 = a > b;
    let cmp6 = a >= b;
    let cmp7 = a < b;
    let cmp8 = a <= b;

    // load/store op
    // variable load/store
    let q = a; // load
    a = 20; // store

    // object property access
    let obj = {x: 5, y: 10};
    let r = obj.x; // property load
    obj.y = 15; // property store

    // array element access
    let arr = [1, 2, 3];
    let s = arr[0]; // array load
    arr[1] = 99; // array store

    // type conversion (no basic block)
    let t = Number("42");
    let u = String(123);
    let v = Boolean(0);

    // other operators (no basic block)
    let w = typeof a; // typeof operator
    let x = void a; // void operator
    let y = (a, b, c); // comma operator
    let z = delete obj.x; // delete operator (on configurable property)

}

ArkTools.arkSteedCompileSync(basic_op);

// Spin loop: A temporary approach to wait for ArkSteed JIT compiler

// TODO: Replace the spin loop above to:
// let res = ArkTools.waitJitCompileFinish(basic_op);
// print(res);

basic_op();
