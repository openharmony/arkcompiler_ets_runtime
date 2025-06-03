/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class Matrix {
    A;
    m;
    n;

    constructor(A) {
        this.A = null;
        this.m = 0;
        this.n = 0;
        (() => {
            this.m = A.length;
            this.n = A[0].length;
            this.A = A;
        })();
    }

    timesEquals(s) {
        for(let i = 0; i < this.m; i++) {
            for(let j = 0; j < this.n; j++) {
                // check this stobjbyvalue
                this.A[i][j] = s * this.A[i][j];
            };
        };
        return this;
    }
}

let A;
function test() {
    let array = []
    for (let i = 0; i < 10; i++) {
        let arr = []
        for (let j = 0; j < 10; j++) {
            arr.push((Math.random() * 10).toFixed())
        }
        array.push(arr)
    }
    A = new Matrix(array);
    A.timesEquals(2);
}

test()
// expected poly state
// slotid : 17, ickind: storeic
print(ArkTools.getICState(A.timesEquals, 17, 3))


{
    function prototypeic() {
        let a = [1, 2];
        for (let i = 3; i < 10; ++i) {
            // test prototype store ic handler
            a[i] = 4;
        }
    }

    prototypeic();

    print(ArkTools.getICState(prototypeic, 2, 3));
}

{
    function transwithprotohandler() {
        for (let i = 0; i < 10; ++i) {
            let a = [1, 2];
            for (let j = 3; j < 10; ++j) {
                // test transwithproto store ic handler when j is 3
                a[j] = 4;
            }
        }
    }

    transwithprotohandler();

    print(ArkTools.getICState(transwithprotohandler, 3, 3));
}