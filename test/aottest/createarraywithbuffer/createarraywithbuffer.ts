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

declare function assert_equal(a: Object, b: Object):void;
var arrayIterator = ['fifth', 'sixth', 666];
assert_equal(arrayIterator[0], "fifth");
assert_equal(arrayIterator[1], "sixth");
assert_equal(arrayIterator[2], 666);

var testNum1 = -1;
var testNum2 = -1;
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
        testNum1 = this.computeScore(newArray);
        testNum2 = this.computeScore(this.currentArrays);
        this.currentArrays = newArray;
    }
}

let index = new Index;
for (let i = 0; i < 3; i++) {
    index.run();
    assert_equal(testNum1, 16 * (i + 1));
    assert_equal(testNum2, 16 * i);
}

let K:number[] = [];
K.push(8.2);
assert_equal(K[0], 8.2);
K[1] = 3;
assert_equal(K[1], 3);

let x = 1.2;
let y = 9;
let T:number[] = [0, 1, 1.2, x];
assert_equal(T[0], 0);
assert_equal(T[1], 1);
assert_equal(T[2], 1.2);
assert_equal(T[3], x);
x = 1;
let Ta:number[] = [,, 4.2, x];
let Tb:number[] = [1, y, 1.2, x];
let Tc:number[] = [-2, -9, 8.3, x];

assert_equal(Ta[0], undefined);
assert_equal(Ta[1], undefined);
assert_equal(Ta[2], 4.2);
assert_equal(Ta[3], 1);

assert_equal(Tb[0], 1);
assert_equal(Tb[1], 9);
assert_equal(Tb[2], 1.2);
assert_equal(Tb[3], 1);

assert_equal(Tc[0], -2);
assert_equal(Tc[1], -9);
assert_equal(Tc[2], 8.3);
assert_equal(Tc[3], 1);

let z = {test: 1.8}

let Td:number[] = [8848, "aotTest", z, x];

assert_equal(Td[0], 8848);
assert_equal(Td[1], "aotTest");
assert_equal(Td[2].test, 1.8);
assert_equal(Td[3], 1);

Td[4] = 9999;
assert_equal(Td[4], 9999);