/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str:any):string;
var arrayIterator = ['fifth', 'sixth', 666];
print(arrayIterator[0]);
print(arrayIterator[1]);
print(arrayIterator[2]);

class Index {
    currentArrays: number[][] = [
        [0, 0, 0, 0],
        [0, 0, 0, 0],
        [0, 0, 0, 0],
        [0, 0, 0, 0]
      ]

    changeCurretArrays() {
        let newArrays = [
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0]
        ]

        for (let j = 0; j < 4; j++) {
            for (let i = 0; i < 4; i++) {
                newArrays[j][i] = this.currentArrays[j][i] + 1;
            }
        }
        return newArrays;
    }

    computeScore(array) {
        let total = 0;
        for (let j = 0; j < 4; j++) {
            for (let i = 0; i < 4; i++) {
                total  += array[j][i];
            }
        }
        return total;
    }

    run() {
        let newArray = this.changeCurretArrays();
        print(this.computeScore(newArray));
        print(this.computeScore(this.currentArrays));
        this.currentArrays = newArray;
    }
}

let index = new Index;
for (let i = 0; i < 3; i++) {
    index.run();
}

let K:number[] = [];
K.push(8.2);
print(K[0]);
K[1] = 3;
print(K[1]);

let x = 1.2;
let y = 9;
let T:number[] = [0, 1, 1.2, x];
print(T[0]);
print(T[1]);
print(T[2]);
print(T[3]);
x = 1;
let Ta:number[] = [,, 4.2, x];
let Tb:number[] = [1, y, 1.2, x];
let Tc:number[] = [-2, -9, 8.3, x];

print(Ta[0]);
print(Ta[1]);
print(Ta[2]);
print(Ta[3]);

print(Tb[0]);
print(Tb[1]);
print(Tb[2]);
print(Tb[3]);

print(Tc[0]);
print(Tc[1]);
print(Tc[2]);
print(Tc[3]);

let z = {test: 1.8}

let Td:number[] = [8848, "aotTest", z, x];

print(Td[0]);
print(Td[1]);
print(Td[2].test);
print(Td[3]);

Td[4] = 9999;
print(Td[4]);