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

declare function print(a0:any, a1?:any):string;

class A {
    x:number;
    y:number;

    constructor(x:number, y:number) {
        this.x = x;
        this.y = y;
    }

    foo() {
        print("A foo");
    }

    bar() {
        print("A bar");
    }

    get t():string {
        return this.constructor.name;
    }

    set t(str:string) {
        print(str);
    }

}

class B extends A {
    z:string;
    constructor(x:number, y:number, z:string) {
        super(x, y);
        this.z = z;
    }

    foo() {
        print("B foo");
    }
}

function testVtable(o:A) {
    print(o.x);
    print(o.y);
    o.foo();
    o.bar();
    print("constructor.name:", o.t);
    o.t = "setter";
}

let a = new A(1, 2);
testVtable(a);

let b = new B(3, 4, "BBBB");
testVtable(b);
print(b.z);
