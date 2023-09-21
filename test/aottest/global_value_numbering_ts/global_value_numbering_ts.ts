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

declare function print(n:string):string;
declare function print(n:number):string;

// CMP
//[compiler] Found a replaceable node, before -> after
//[compiler] {"id":212, "op":"ICMP", "MType":"I1, bitfield=0xa, type=NJS_VALUE-GT(M=0, L=0), stamp=10, mark=2, ","in":[[], [], [55, 205], [], []], "out":[213]},
//[compiler] {"id":209, "op":"ICMP", "MType":"I1, bitfield=0xa, type=NJS_VALUE-GT(M=0, L=0), stamp=10, mark=3, ","in":[[], [], [55, 205], [], []], "out":[210]},

//[compiler] {"id":55, "op":"VALUE_SELECTOR", "MType":"I32, bitfield=0x0, type=NJS_VALUE-GT(M=0, L=0), stamp=9, mark=3, ","in":[[16], [], [227, 217], [], []], "out":[215, 212, 209, 206, 102, 88, 94, 97, 99, 85, 83, 80, 71]},
//[compiler] {"id":205, "op":"CONSTANT", "MType":"I32, bitfield=0x1, type=NJS_VALUE-GT(M=0, L=0), stamp=9, mark=3, ","in":[[], [], [], [], []], "out":[227, 224, 221, 215, 212, 209]},

for (var i:number = 0; i < 10; i++) {
      if (i <= 1) {
        print("Hello");
      }
      if (i <= 1) {
        print("Hello");
      }
}

// TypedOperator

//[compiler] Found a replaceable node, before -> after
//[compiler] {"id":669, "op":"FDIV", "MType":"F64, bitfield=0x0, type=NJS_VALUE-GT(M=0, L=0), stamp=10, mark=2, ","in":[[], [], [608, 610], [], []], "out":[670]},
//[compiler] {"id":664, "op":"FDIV", "MType":"F64, bitfield=0x0, type=NJS_VALUE-GT(M=0, L=0), stamp=10, mark=3, ","in":[[], [], [608, 610], [], []], "out":[665]},
function div(a: number, b: number): number {
    let sum = 0;
    if (a > 0) {
        let k = a / b;
        sum += k;
    }
    if (b > 0) {
        let k = a / b;
        sum += k;
    }
    return sum;
}
print(div(1,2));

function mul(a: number, b: number): number {
    let sum = 0;
    if (a > 0) {
        let k = a * b;
        sum += k;
    }
    if (b > 0) {
        let k = a * b;
        sum += k;
    }
    return sum;
}
print(mul(1,2));

function sub(a: number, b: number): number {
    let sum = 0;
    if (a > 0) {
        let k = a - b;
        sum += k;
    }
    if (b > 0) {
        let k = a - b;
        sum += k;
    }
    return sum;
}
print(sub(1,2));


function add(a: number, b: number): number {
    let sum = 0;
    if (a > 0) {
        let k = a + b;
        sum += k;
    }
    if (b > 0) {
        let k = a + b;
        sum += k;
    }
    return sum;
}
print(add(1,2));

