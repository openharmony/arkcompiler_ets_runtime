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

class Matrix {
    A;
    
    constructor(A) {
        (() => {this.A = A;})();
    }
    
    getArray() {
        return this.A;
    }
}
    
let B;

function getArray_test() {
    let array = [];
    let A;
    for (let i = 0; i < 500; i++) {
        array = [[1, 2, 3, 4, 5], [5, 4, 3, 2, 1]];
        A = new Matrix(array);
        B = A.getArray();
    }
}
    
getArray_test();
ArkTools.waitAllJitCompileFinish();
getArray_test();
print(JSON.stringify(B));