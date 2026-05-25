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

    constructor(a: number, b: number, c: number) {
        super(a, b);
        this.c = c;
    }

    total(): number {
        return this.a + this.b + this.c;
    }
}

function spread_constructor_inheritance_1(
    base: [number, number], derived: [number, number, number]): number {
    const p = new Parent(...base);
    const c = new Child(...derived);
    return p.sum() + c.total();
}

ArkTools.arkSteedCompileSync(Parent);
ArkTools.arkSteedCompileSync(Child);
ArkTools.arkSteedCompileSync(spread_constructor_inheritance_1);


print(spread_constructor_inheritance_1([1, 2], [10, 20, 30]));
