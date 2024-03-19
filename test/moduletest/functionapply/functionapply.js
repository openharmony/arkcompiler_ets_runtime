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

function createArray(len) {
    return Array.apply(null, {length: len});
}
let arr = createArray(10);
let a1 = createArray(23.2);
let a2 = createArray();
print(arr);
print(a1);
print(a2.length);
const v1 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
const v4 = Int8Array.from(v1, v5 => v5.charCodeAt(0));
Object.defineProperty(v4, "length", {
    value:0
});
print(String.fromCharCode.apply(null, v4));
function f0(a, b) {
    print(a,b);
}

let v38;
function f2() {
    arguments.length = -1;
    v38 = arguments;
}
f2(1,2);
f0.apply(null,v38);