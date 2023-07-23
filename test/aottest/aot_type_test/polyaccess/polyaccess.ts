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

declare function print(arg: any):string;

class C {
    x:number = 1;
    y:number = 11;
}

class D {
    y:number = 22;
    x:number = 2;
}

function foo(o1: C | D, o2: C) {
    // poly
    print(o1.x);
    o1.y = 3;
    print(o1.y);

    // mono
    print(o2.x);
}

function bar(o1: C, o2: C | D) {
    // mono
    print(o1.x);

    // poly
    print(o2.x);
    o2.y = 33;
    print(o2.y);
}

function test() {
    let c = new C();
    let d = new D();
    foo(c, c);
    foo(d, c);
    // @ts-ignore
    foo({}, c);  // will deopt after 2 comparisons

    bar(c, c);
    bar(c, d);
    // @ts-ignore
    bar(c, {});  // will deopt after 2 comparisons
}

test();

