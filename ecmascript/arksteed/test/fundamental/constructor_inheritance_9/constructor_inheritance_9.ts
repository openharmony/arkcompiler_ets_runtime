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
class Base {
    a: number;

    constructor(a: number) {
        this.a = a;
    }

    getValue(): number {
        return this.a;
    }
}

class Left extends Base {
    b: number;

    constructor(a: number, b: number) {
        super(a);
        this.b = b;
    }

    getAB(): number {
        return this.a + this.b;
    }
}

class Right extends Base {
    c: number;

    constructor(a: number, c: number) {
        super(a);
        this.c = c;
    }

    getAC(): number {
        return this.a + this.c;
    }
}

class Combined extends Left {
    d: number;

    constructor(a: number, b: number, c: number, d: number) {
        super(a, b);
        this.c_value = c;
        this.d = d;
    }

    c_value: number;

    getABCD(): number {
        return this.a + this.b + this.c_value + this.d;
    }
}

ArkTools.arkSteedCompileAsync(Base);
ArkTools.arkSteedCompileAsync(Left);
ArkTools.arkSteedCompileAsync(Right);
ArkTools.arkSteedCompileAsync(Combined);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let obj = new Combined(1, 2, 3, 4);
print(obj.getValue());
print(obj.getAB());
print(obj.getABCD());
