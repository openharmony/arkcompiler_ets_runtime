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

class Foo {
    x: number;
    limit: number;
    count: number;

    constructor(x: number, limit: number) {
        this.x = x;
        this.limit = limit;
        this.count = 0;
    }

    iterate() {
        if (this.x % 2 == 0) {
            this.x /= 2;
        } else {
            this.x = this.x * 3 + 1;
        }
        this.count += 1;
        return this.count;
    }

    getCount() {
        let n: number;
        do {
            n = this.iterate();
        } while (this.x > 1 && n < this.limit);
        return n;
    }
}

ArkTools.arkSteedCompileAsync(Foo.prototype.getCount);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = new Foo(1, 100).getCount();
print(output);
output = new Foo(10, 100).getCount();
print(output);
output = new Foo(100, 100).getCount();
print(output);
output = new Foo(1000, 100).getCount();
print(output);
