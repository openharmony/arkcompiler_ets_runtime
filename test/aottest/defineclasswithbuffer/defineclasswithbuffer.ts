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

declare function print(str:any):string;
declare function assert_unreachable():void;
declare function assert_true(condition: boolean):void;
declare function assert_equal(a: Object, b: Object):void;
class Obj1 {
    value:number;
    constructor(value:number) {
        this.value = value;
    }
    fun(s:number):number {
        return 1 + this.value + s;
    }
}

class Obj2 {
    fun():void {
        print("Hello World");
    }
}

var obj1 = new Obj1(1);
assert_equal(obj1.value, 1);
assert_equal(obj1.fun(2), 4);
var obj2 = new Obj2();
obj2.fun();

assert_true(obj1 instanceof Object);

// The following test case once exposed a bug: The abstract methods were not ignored when generating phc using ts types,
// resulting in the inconsistency between the properties key and class litreal at runtime.
abstract class C {
    constructor() {}

    abstract foo(): void;

    bar(): string {
       return "bar"
    }
}

assert_equal(C.prototype.bar(), "bar");

function f26() {
    function f29() {}
    f29();
    throw f29
}
try {
    const v31 = f26();
    assert_unreachable();
} catch (e) {}
class C33 {}