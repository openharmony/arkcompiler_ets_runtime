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

// Test: throw.ifsupernotcorrectcall no-throw path (both index 0 and index 1).
// - index 0x1: super() is called exactly once, this was undefined → no throw
// - index 0x0: this is accessed after super(), this is bound → no throw

class Base1 {
    x: number;
    constructor(x: number) {
        this.x = x;
    }
}

class Derived1 extends Base1 {
    y: number;
    constructor(x: number, y: number) {
        super(x);          // throw.ifsupernotcorrectcall 0x1: this is undefined → no throw
        this.y = y;        // throw.ifsupernotcorrectcall 0x0: this is bound → no throw
    }
}

let cnt = 0;

function test(): void {
    let d = new Derived1(10, 20);
    if (d.x === 10 && d.y === 20) {
        cnt++;
    }
}

ArkTools.arkSteedCompileSync(test);
test();
print(cnt);
