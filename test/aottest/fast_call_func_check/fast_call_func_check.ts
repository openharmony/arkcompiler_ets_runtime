/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
declare function print(str:any):string;

function F0() {
    this.g = 7;
}
class C6 {
    constructor(a8) {
        this.p = a8;
        try { this.p(new F0()); } catch (e) {}
    }
}

new C6(new F0);
new C6(F0);
print(1);

class Bar {
    constructor(a) {
        this.p = a;
    }
}
function foo1(a: number): number {
    return a + 1;
}
function callFoo(f, a: number): number {
    return f(a);
}
callFoo(foo1, 1);
try {
    callFoo(new Bar(foo1), 2);
} catch (e) {}

print(2);

function* foo2(i: number) {
    yield i;
}
function callFoo2(f, a: number): number {
    let gen = f(a);
    return gen.next().value;
}
callFoo2(foo2,1);
try {
    callFoo2(new Bar(foo2), 2);
} catch (e) {}

print(3);

const a = {
    foo1 : function*() {
        yield 1;
    }
}
var func = a.foo1;
print(func().next().value);