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

    constructor(a: number, b: number) {
        this.a = a;
        this.b = b;
    }

    sum(): number {
        return this.a + this.b;
    }
}

class Child extends Parent {
    c: number;
    rest: number[];

    constructor(a: number, b: number, ...rest: number[]) {
        super(...[a, b]);
        this.c = rest[0];
        this.rest = rest.slice(1);
    }

    total(): number {
        let s = this.a + this.b + this.c;
        for (let i = 0; i < this.rest.length; i++) {
            s += this.rest[i];
        }
        return s;
    }
}

function spread_constructor_inheritance_5(nums: number[]): number {
    const p = new Parent(...nums);
    const c = new Child(...nums);
    return p.sum() + c.total();
}

ArkTools.arkSteedCompileSync(Parent);
ArkTools.arkSteedCompileSync(Child);
ArkTools.arkSteedCompileSync(spread_constructor_inheritance_5);


print(spread_constructor_inheritance_5([1, 2, 3, 4, 5]));
