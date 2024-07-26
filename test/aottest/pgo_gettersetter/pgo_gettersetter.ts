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

let arkPrivate = globalThis.ArkPrivate;
arkPrivate.Load(arkPrivate.Stack);
arkPrivate.Load(arkPrivate.HashMap);

function bgc() {
    for (let i=0; i < 0x10000; i+=1) {
        new String();
    }
    let a = new Array(0x10000);
    for (let i=0; i<0x10000; i+=1) {
        a[i] = new String(0x100);
    }
}
function sgc() {
    for(let i=0; i<0x10000; i+=1) {
        new String();
    }
}

function runNearStackLimit(f) {
    function t() {
        try { 
            t();
        } catch (e) {
            f();
        }
    };
    try {
        t();
    } catch(e) {} 
}
function arkPrint(d) {
    print(JSON.stringify(d));
}
const v1 = ["15254"];
for (const v2 of v1) {
    const o3 = {};
    function f4() {
        return v1;
    }
    class C5 extends f4 {
        set e(a7) {}
    }
    const o9 = {
        get e() {
            return f4;
        }
    };
    for (let v10 = 0; v10 < 100; v10++) {
        [-152.83464104903533,2.2250738585072014e-308,773563.4380633899,0.598964827003518,4.0,4.0,-5.0,-378087.9967490771,1000000000000.0,-5.0];
    }
    o3.c=v2;
}
print(123);
