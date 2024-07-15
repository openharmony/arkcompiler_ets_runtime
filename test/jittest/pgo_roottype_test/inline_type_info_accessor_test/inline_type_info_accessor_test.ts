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

function f3() {
    return 268435440n;
}
function f4() {
    return 53315n;
}
new Date();
new Int16Array(3866);
const v12 = new Int16Array(10);
const v15 = new Float64Array(4);
for (let i18 = 9, i19 = 10;
    (() => {
        const v20 = i19--;
        for (let i23 = 2, i24 = 10; i24-- < i24;) {
        }
        return v20;
    })();
    (() => {
        for (let [i43, i44] = (() => {
            function f33(a34, a35, a36) {
                const o37 = {
                    "b": Int16Array,
                    "f": i19,
                    "h": Int16Array,
                };
                return o37;
            }
            f33(268435440n, 3866, 268435440n);
            f33(268435440n, i19, 268435440n);
            f33(53315n, 4, -770327790n);
            return [-1024, 58693];
        })();
            i44--;
        ) {
        }
    })()) {
}
for (let i54 = -15651, i55 = 10;
    i55--, i54 < i55;
    (() => {
        const o60 = {
            "call": Date,
            "deleteProperty": Date,
            "getPrototypeOf": f4,
        };
        new Proxy(v12, o60);
    })()) {
}
const v65 = [9, 9, 9, 9, 9];
const v68 = new Uint16Array(512);
function f69(a70, a71, a72, a73) {
    for (const v74 of v15) {
        const o77 = {
            "maxByteLength": 82,
        };
        const v79 = new SharedArrayBuffer(82, o77);
        new Uint16Array(v79);
    }
    a73[179] = a73;
    const t59 = v65.constructor;
    const v83 = t59(a72);
    try { v83.find(f69); } catch (e) { }
    const v86 = 1139204126 >> a72;
    for (let i89 = 0, i90 = 47940; v86 < i90; i90--) {
    }
    return v83;
}
v68.reduceRight(f69);
print("execute successful.")