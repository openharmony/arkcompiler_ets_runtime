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

declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
declare function print(arg:any):string;

let v1 = new Map();
let v2 = new Set();
let v3 = new Date();
class C {
    constructor(a4, a5, a6) {
        let v11 = v1;
        let v22 = v2;
        let v33 = v3;

        v1 ^= a4;
        v2 ^= a5;
        v3 ^= a6;

        v11["has"](1);
        v22["has"](2);
        v33["getTime"]();
    }
}

function testCallSpecificObjects(): void {
    const c1 = new C(Map, Set, Date);
    try {
        const c2 = new C(1, 1, 1);
    } catch (e) { }
}

testCallSpecificObjects();
print("testCallSpecificObjects success, isAotCompiled: " + ArkTools.isAOTCompiled(testCallSpecificObjects));

class B {
    constructor() {}
};
function f() {
    for (let i = 0; i < 4; i++) {
        let A;
        if (i&1) A = B;
        else A = Object;
        let a = new A();
    }
}

f()
print(ArkTools.isAOTDeoptimized(f));