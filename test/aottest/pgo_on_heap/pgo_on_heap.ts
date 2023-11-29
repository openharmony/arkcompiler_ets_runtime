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

declare function print(arg:any, arg1?: any):string;

const ON_HEAP_LENGTH = 100;
const NOT_ON_HEAP_LENGTH = 5000;

let ta0 = new Uint32Array(ON_HEAP_LENGTH);
for(let i: number = 0; i < ON_HEAP_LENGTH; i++) {
    ta0[i] = i;
}

let ta1 = new Float64Array(NOT_ON_HEAP_LENGTH);
for(let i: number = 0; i < NOT_ON_HEAP_LENGTH; i++) {
    ta1[i] = i;
}

// on heap
function foo(ta: Uint32Array) {
    let res = 0;
    for(let i: number = 0; i < ON_HEAP_LENGTH; i++) {
        res += ta[i];
    }
    print(res);
}
foo(ta0);

// not on heap
function bar(ta: Float64Array) {
    let res = 0;
    for(let i: number = 0; i < NOT_ON_HEAP_LENGTH; i++) {
        res += ta[i];
    }
    print(res);
}
bar(ta1);


// none
function baz(ta: Uint32Array | Float64Array) {
    let res = 0;
    for(let i: number = 0; i < ON_HEAP_LENGTH; i++) {
        res += ta[i];
    }
    print(res);
}
baz(ta0);
baz(ta1);

