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
    foo: number;

    constructor(foo: number) {
        this.foo = foo;
    }
}

class Derived extends Base {
    bar: number;

    constructor(foo: number, bar: number) {
        super(foo);
        this.bar = bar;
    }
}

// Compile both
ArkTools.arkSteedCompileSync(Base);
ArkTools.arkSteedCompileSync(Derived);


let d = new Derived(24, 36);
print(d.foo);
print(d.bar);
