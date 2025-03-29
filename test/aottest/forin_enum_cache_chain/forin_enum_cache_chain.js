/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

{
    print("====Test 0: start====");
    function Print(obj) {
        for (let i in obj) {
            print(i);
        }
        print();
    }
    class A{

    };
    class B extends A{

    };
    let b = new B();
    b.x = 1;
    b.y = 1;

    Print(b);
    Print(b);

    A.prototype.z = 1;
    // This used to enable proto changemarker.
    for (let i = 0 ; i < 50; i++) {
        b.z;
    }
    Print(b);
    Print(b);

    print("=====Test 0: end=====");
}

{
    function Print(obj) {
        for (let i in obj) {
            print(i);
        }
        print();
    }
    class A{

    };
    class B extends A{

    };
    B.prototype.u = 1;
    B.prototype.v = 1;
    A.prototype.o = 1;
    let b1 = new B();
    let b2 = new B();
    let b3 = new B();
    
    print("====Test 1: start====");
    Print(b1);
    b2.p = 1;
    Print(b2);
    b3[1] = 1;
    b3[2] = 1;
    Print(b3);
    print("=====Test 1: end=====");
    
    
    B.prototype.z = 1;
    // This used to enable proto changemarker.
    for (let i = 0 ; i < 50; i++) {
        b1.z;
    }

    print("====Test 2: start====");
    Print(b1);
    delete B.prototype.u;
    Print(b1);
    Print(b2);
    Print(b3);
    for (let i in b1) {
        delete A.prototype.o;
        print(i);
    }
    print();
    print("=====Test 2: end=====");

    print("====Test 3: start====");
    Print(b1);
    Print(b2);
    Print(b3);
    Print(b1);
    Print(b2);
    Print(b3);
    print("=====Test 3: end=====");

    print("====Test 4: start====");
    Print(b1);
    Print(b2);
    Print(b3);
    Print(b1);
    Print(b2);
    Print(b3);
    print("=====Test 4: end=====");


    A.prototype[6] = 1;
    print("====Test 5: start====");
    Print(b1);
    Print(b2);
    Print(b3);
    print("=====Test 5: end=====");

    B.prototype[5] = 1;
    print("====Test 6: start====");
    Print(b1);
    Print(b2);
    Print(b3);
    print("=====Test 6: end=====");
}

{
    print("====Test 7: start====");
    class A{

    }
    class B extends A{

    }
    class C extends B{

    }
    let c = new C();
    c.x = 1;
    for (let i in c) {
        print(i);
    }
    A.prototype.y = 1;
    for (let i in c) {
        print(i);
    }
    print("=====Test 7: end=====");
}
{
    print("====Test 8: start====");
    class A{
    }
    let a = new A();
    for (let i = 0; i < 2; i++) {
        for (let j in a) {
            print(j);
        }
    }
    a.x = 1;
    delete a.x;
    for (let i = 0; i < 2; i++) {
        for (let j in a) {
            print(j);
        }
    }
    print("=====Test 8: end=====");
}
{
    print("====Test 9: start====");
    class A{
    }
    let a = new A();
    a.x = 1;
    a.y = 2;
    a.__proto__ = 1;
    for (let i = 0; i < 2; i++) {
        for (let j in a) {
            print(j);
        }
    }
    a.z = 3;
    a.__proto__ = "test";
    for (let i = 0; i < 2; i++) {
        for (let j in a) {
            print(j);
        }
    }
    print("=====Test 9: end=====");
}