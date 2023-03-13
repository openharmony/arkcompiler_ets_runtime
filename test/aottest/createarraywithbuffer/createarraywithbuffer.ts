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