/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

let str = "0";
function A() {
    for (let j = 0; j < 2; j++) {
        str = str + j;
    }
    let arr = new Array();
    for (let i = 0; i < 2; i++) {
        arr.push(new Error());
    }
    arr = null;
    ArkTools.forceFullGC();

    arr = new Array();
    for (let i = 0; i < 2; i++) {
        arr.push(new Error());
    }
    arr = null;

}

function B() {
    let arr = new Array();
    for (let i = 0; i < 2; i++) {
        arr.push(new Error());
    }
    A();
}

function C(array) {
    B();

    let i = 0;
    let len = array.length;

    for (let j = 0; j < len; j++) {
        i = i + array[j];
    }

    return i;
}

const array = new Array();
for (let i = 0; i < 2; i++) {
    array.push(i);
}

C(array);
ArkTools.jitCompileAsync(A);
ArkTools.jitCompileAsync(B);
ArkTools.jitCompileAsync(C);
var compilerResult = ArkTools.waitJitCompileFinish(C);

let str1 = "ArkTools"

C(str1)
print(compilerResult)
