/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

declare function print(arg:any):string;

function A() {
    this.x = 1;
}

A.prototype.foo = function() {
    print("foo");
}

A.prototype.bar = function() {
    print("bar");
}

let a = new A();
print(a.x);
a.x =2;
print(a.x);
a.foo();
a.bar();


function B() {
    this.x = 1;
}

function foo(b) {
    print(b.bar);
}

let b = new B();
foo(b);

B.prototype.bar = "bar";
foo(b);



for(let i = 0; i<2; ++i) {
    function C() {
        this.x = 1;
    }
    let c = new C();
    print(c.x);
}

