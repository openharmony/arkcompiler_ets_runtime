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
class L0 {
    a: number;

    constructor(a: number) {
        this.a = a;
    }

    getA(): number {
        return this.a;
    }
}

class L1 extends L0 {
    b: number;

    constructor(a: number, b: number) {
        super(a);
        this.b = b;
    }

    getB(): number {
        return this.b;
    }
}

class L2 extends L1 {
    c: number;

    constructor(a: number, b: number, c: number) {
        super(a, b);
        this.c = c;
    }

    getC(): number {
        return this.c;
    }
}

class L3 extends L2 {
    d: number;

    constructor(a: number, b: number, c: number, d: number) {
        super(a, b, c);
        this.d = d;
    }

    total(): number {
        return this.a + this.b + this.c + this.d;
    }
}

ArkTools.arkSteedCompileSync(L0);
ArkTools.arkSteedCompileSync(L1);
ArkTools.arkSteedCompileSync(L2);
ArkTools.arkSteedCompileSync(L3);


let obj = new L3(1, 2, 3, 4);
print(obj.getA());
print(obj.getB());
print(obj.getC());
print(obj.total());
