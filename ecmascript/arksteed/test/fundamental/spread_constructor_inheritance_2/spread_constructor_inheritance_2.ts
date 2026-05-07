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
class Parent {
    a: number;
    b: number;
    c: number;

    constructor(...nums: number[]) {
        this.a = nums[0];
        this.b = nums[1];
        this.c = nums[2];
    }

    sum(): number {
        return this.a + this.b + this.c;
    }
}

class Child extends Parent {
    d: number;

    constructor(...nums: number[]) {
        super(...nums);
        this.d = nums[3];
    }

    total(): number {
        return this.a + this.b + this.c + this.d;
    }
}

function spread_constructor_inheritance_2(nums: number[]): number {
    const p = new Parent(...nums);
    const c = new Child(...nums);
    return p.sum() + c.total();
}

ArkTools.arkSteedCompileAsync(Parent);
ArkTools.arkSteedCompileAsync(Child);
ArkTools.arkSteedCompileAsync(spread_constructor_inheritance_2);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(spread_constructor_inheritance_2([1, 2, 3, 4]));
