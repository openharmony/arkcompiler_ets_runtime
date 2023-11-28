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

for (var i:number = 0; i < 10; i++) {
      if (i <= 1) {
        print("Hello");
      }
      if (i <= 1) {
        print("Hello");
      }
}

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

