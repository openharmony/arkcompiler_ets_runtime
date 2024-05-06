/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(arg: any): string;

class A {
    #a_: number;
    constructor() {
        this.#a_ = 0;
    }
    get #a() {
        return this.#a_;
    }
    set #a(v) {
        this.#a_ = v;
    }
    foo() {
        return this.#a;
    }
    bar() {
        this.#a = 1;
    }
}

for (let i = 0; i < 5; i++) {
    let a = new A();
    print(a.foo());
    a.bar();
    print(a.foo());
}

// hcdata with object (phc)
class B {}
B.prototype.foo = function () {
    return 1;
};

// primitive symbol no pgo
const symbol = Symbol();
const namedSymbol = Symbol("symbol");
print(symbol.toString());
print(namedSymbol.toString());
