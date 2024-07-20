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
declare var ArkTools:any;
var b:Boolean = new Boolean(2);
print(b);

var n:Number = new Number(123);
print(n);

var o:Object = new Object("newobj");
print(o);

var a:Array<number> = new Array(10);
print(a.length);

class TClass {
    dd: number;

    div(v1: number[]): number {
        return 1.0 / v1[0];
    }
    constructor(p1: number[]) {
        let d = this.div(p1);
        this.dd = d / 1.0;
    }
}

let p1: number[] = [-10, 10, -10];
new TClass(p1);

function foo() {
    print(new.target.name);
    return "pass";
}

function fun() {
    return "pass";
}

function foo1(a: any) {
    print(new.target.name);
    return a;
}

function func1(value: number): number {
    print(new.target.name);
    print(value);
    return value;
}

function funcv(value: number, value1: number, value2: number, value3: number, value4: number, value5: number, value6: number, value7: number): number {
    print(new.target.name);
    print(value);
    print(value1);
    print(value2);
    print(value3);
    print(value4);
    return 100;
}

class A {
    public mode: number = 1;

    constructor() {
        print("A:constructor!");
        print(new.target.name);
        const size = 50;
        this.mode = 4;
    }

    update (): void {
        print(new.target);
        print("A update");
    }
}

class D extends A {
    public len: number = 1;

    constructor() {
        super();
        super.update();
        print("D:constructor!");
        print(new.target.name);
        const size = 50;
        this.len = 10;
    }
}

class B {
    public mode: number = 1;

    constructor(dt: number, dt1: number, dt2: number) {
        print("B:constructor!");
        print(dt);
        print(dt1);
        print(new.target.name);
        const size = 50;
        this.mode = 4;
    }
    
    update (dt: number, dt1: number): void {
        print(new.target);
        this.mode = dt1;
        print(dt);
    }
}

class C extends B {
    public color: number = 5;

    constructor(dt: number, dt1: number, dt2: number) {
        super(dt, dt1, dt2);
        super.update(dt, dt1);
        print("C:constructor!");
        print(new.target.name);
        this.color = dt2;
    }
}


function funcAsm(value: number, value1: number, value2: number): number {
    print(new.target.name);
    print(value);
    print(value1);
    var s: A = new A(1, 4);
    var s1: B = new B(2, 3);
    return value;
}


function Parent(name: string): void{
    print(new.target);
    this.name = name;
    this.arr = [1,2,3];
}

// 2
function Child(name: string, age: number): void {
    print(new.target.name);
    Parent.call(this, name)
    this.age = age;
}

var systems: A = new A(1);
var systems1: B = new B(2, 3);
var systems1: D = new D();
var systems2: C = new C(4, 5);
new foo();
new foo1(new Boolean(2));
new func1(1);
new funcAsm(4, 5, 6);
new funcv(0, 1, 2, 3, 4, 5, 6, 7);
new funcv(0, 1, 2, 3, 4, 5);
new Child("jjj", 15);
print(ArkTools.isAOTCompiled(fun));
new fun();
print(ArkTools.isAOTCompiled(fun));
print(ArkTools.isAOTCompiled(foo));
print(ArkTools.isAOTCompiled(foo1));
print(ArkTools.isAOTCompiled(funcAsm));
print(ArkTools.isAOTCompiled(funcv));
print(ArkTools.isAOTCompiled(Child));
print(ArkTools.isAOTCompiled(Parent));
print(ArkTools.isAOTCompiled(A));
print(ArkTools.isAOTCompiled(D));
print(ArkTools.isAOTCompiled(C));
print(ArkTools.isAOTCompiled(D));
