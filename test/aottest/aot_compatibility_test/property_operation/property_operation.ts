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

declare function print(arg:any):string;

{
    class A {
        x?: number;
        x2?: number;
        constructor(x: number) {
            this.x = x;
        }

        foo() {
            print("xxxx");
        }
    }

    class B extends A {
        y?: number;
        constructor(x: number, y: number) {
            super(x);
            this.y = y;
        }
    }

    // instance define Property
    let b1 = new B(1, 2);
    print(b1.hasOwnProperty("x1"));
    Object.defineProperty(b1, "x1", {value:1});
    print(b1.hasOwnProperty("x1"));

    // instance delete and change Property
    let b2 = new B(1, 2);
    print(b2.hasOwnProperty("y"));
    print(b2.y);
    b2.y = 3;
    print(b2.y);
    delete b2.y;
    print(b2.hasOwnProperty("y"));

    // prototype define Property
    let p = A.prototype;
    let b3 = new B(1, 2);
    print(b3.x2);
    print(Reflect.has(b3, "x2"));
    Object.defineProperty(p, "x2", {value:1});
    print(b3.x2);
    print(Reflect.has(b3, "x2"));

    // prototype delete and change Property
    let p2 = A.prototype;
    let b4 = new B(1, 2);
    print(b4.x);
    b4.x = 3;
    print(b4.x);
    print(b4.hasOwnProperty("x"));
    delete p2.x;
    print(b4.hasOwnProperty("x"));

    // prototype change and call function
    let b5 = new B(1, 2);
    b5.foo();
    Object.setPrototypeOf(b5, {})
    try {
        b5.foo();
    } catch(e) {
        print(e instanceof TypeError);
    }
}
