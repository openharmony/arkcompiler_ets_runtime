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
class GrandParent {
    a: number;
    b: number;

    constructor(a: number, b: number) {
        this.a = a;
        this.b = b;
    }
}

class Parent extends GrandParent {
    c: number;

    constructor(a: number, b: number, c: number) {
        super(a, b);
        this.c = c;
    }
}

class Child extends Parent {
    d: number;

    constructor(a: number, b: number, c: number, d: number) {
        super(a, b, c);
        this.d = d;
    }

    total(): number {
        return this.a + this.b + this.c + this.d;
    }
}

function spread_constructor_inheritance_4(coords: [number, number, number, number]): number {
    const c = new Child(...coords);
    return c.total();
}

ArkTools.arkSteedCompileAsync(GrandParent);
ArkTools.arkSteedCompileAsync(Parent);
ArkTools.arkSteedCompileAsync(Child);
ArkTools.arkSteedCompileAsync(spread_constructor_inheritance_4);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(spread_constructor_inheritance_4([1, 2, 3, 4]));
