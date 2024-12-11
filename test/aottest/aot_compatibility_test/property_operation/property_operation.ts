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

declare function assert_unreachable():void;
declare function assert_true(condition: boolean):void;
declare function assert_equal(a: Object, b: Object):void;
declare function print(arg:any):string;

{
    class A {
        x?: number;
        x2?: number;
        constructor(x: number) {
            this.x = x;
        }

        foo() {
            return("xxxx");
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
    assert_true(!b1.hasOwnProperty("x1"));
    Object.defineProperty(b1, "x1", {value:1});
    assert_true(b1.hasOwnProperty("x1"));

    // instance delete and change Property
    let b2 = new B(1, 2);
    assert_true(b2.hasOwnProperty("y"));
    assert_equal(b2.y, 2);
    b2.y = 3;
    assert_equal(b2.y, 3);
    delete b2.y;
    assert_true(!b2.hasOwnProperty("y"));

    // prototype define Property
    let p = A.prototype;
    let b3 = new B(1, 2);
    assert_equal(b3.x2, undefined);
    assert_true(!Reflect.has(b3, "x2"));
    Object.defineProperty(p, "x2", {value:1});
    assert_equal(b3.x2, 1);
    assert_true(Reflect.has(b3, "x2"));

    // prototype delete and change Property
    let p2 = A.prototype;
    let b4 = new B(1, 2);
    assert_equal(b4.x, 1);
    b4.x = 3;
    assert_equal(b4.x, 3);
    assert_true(b4.hasOwnProperty("x"));
    delete p2.x;
    assert_true(b4.hasOwnProperty("x"));

    // prototype change and call function
    let b5 = new B(1, 2);
    assert_equal(b5.foo(), "xxxx");
    Object.setPrototypeOf(b5, {})
    try {
        b5.foo();
        assert_unreachable();
    } catch(e) {
        assert_equal(e.message, "CallObj is NonCallable");
    }
	
	test_end();
}
