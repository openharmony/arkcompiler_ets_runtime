/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

class A {
    x: number;
    constructor(x: number) { this.x = x; }
}
class B {
    y: number;
    constructor(y: number) { this.y = y; }
}
class C {
    z: number;
    constructor(z: number) { this.z = z; }
}

let obj: A | B | C = new A(0);

function init(x: number) {
    obj = new A(x);
}

function iterate() {
    if (obj instanceof A) {
        obj = new C(obj.x + 1);
    } else if (obj instanceof B) {
        obj = new C(obj.y + 10);
    } else if (obj.z % 11 == 0) {
        obj = new A(obj.z);
    } else if (obj.z % 13 == 0) {
        obj = new B(obj.z);
    } else {
        obj.z += 100;
    }
    return obj;
}

function loop_dowhile_9() {
    let res: A | B | C;
    do {
        res = iterate();
    } while (!(res instanceof A));
    return res;
}

ArkTools.arkSteedCompileAsync(loop_dowhile_9);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

init(1);
print(loop_dowhile_9().x);
init(10);
print(loop_dowhile_9().x);
init(100);
print(loop_dowhile_9().x);
init(1000);
print(loop_dowhile_9().x);
