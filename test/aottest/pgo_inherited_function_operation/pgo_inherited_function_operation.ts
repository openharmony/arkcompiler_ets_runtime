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

declare function print(arg:any):string;
declare var ArkTools:any;
print("======================PART1=======================")
function Test1() {
  function Base() {
    this.c = 0    // store: this.c --> type: MONO_STORE_PROPERTY
    this.x = 1    // store: this.x --> type: MONO_STORE_PROPERTY
  }
  Base.prototype.add = () => { print("add") } // store: Base.prototype.add --> type: MONO_STORE_PROPERTY

  function Derived() {
    this.y = 2    // store: this.y --> type: MONO_STORE_PROPERTY
  }

  let p = new Base()
  p.z = 3         // store: p.z --> type: MONO_STORE_PROPERTY
  print(p.x)      // load: p.x --> type: LOAD_PROPERTY
  print(p.z)      // load: p.z --> type: LOAD_PROPERTY

  Derived.prototype = p
  Derived.prototype.sub = () => { print("sub") } // store: Derived.prototype.sub --> type: MONO_STORE_PROPERTY
  let obj = new Derived()
  obj.w = "w"     // store: obj.w --> type: MONO_STORE_PROPERTY
  obj.x = 4       // store: obj.x --> type: MONO_STORE_PROPERTY

  obj.add()       // load: obj.add --> type: MONO_LOAD_PROPERTY_ON_PROTO
  obj.sub()       // load: obj.sub --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(p.x)      // load: p.x --> type: LOAD_PROPERTY
  print(p.z)      // load: p.z --> type: LOAD_PROPERTY
  print(obj.c)    // load: obj.c --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(obj.x)    // load: obj.x --> type: LOAD_PROPERTY
  print(obj.y)    // load: obj.y --> type: LOAD_PROPERTY
  print(obj.z)    // load: obj.z --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(obj.w)    // load: obj.w --> type: LOAD_PROPERTY
}
Test1();
ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.printTypedOpProfiler("MONO_LOAD_PROPERTY_ON_PROTO");
ArkTools.clearTypedOpProfiler();
print(ArkTools.isAOTDeoptimized(Test1));
print("======================PART2=======================")
function Test2() {
  function Base0() {
    this.x = 1
  }
  function Base1() {
    this.y = 1
  }
  function Derived() {
    this.z = 2
  }

  let p1 = new Base0()
  let p2 = new Base1()
  Derived.prototype = p1  // break the ts chain when try to rebuild it
  Derived.prototype = p2

  let obj = new Derived()
  print(obj.x)
  print(obj.y)
  print(obj.z)
}
Test2();
ArkTools.clearTypedOpProfiler();
print(ArkTools.isAOTDeoptimized(Test2));
print("======================PART3=======================")
function Test3() {
  function Base() {
    this.w = 1    // store: this.x --> type: MONO_STORE_PROPERTY * 2
  }
  Base.prototype.add = () => { print("add") } // store: Base.prototype.add --> type: MONO_STORE_PROPERTY
  function Derived1() {
    this.x = 2    // store: this.y --> type: MONO_STORE_PROPERTY
  }
  function Derived2() {
    this.y = 3    // store: this.y --> type: MONO_STORE_PROPERTY
  }
  function Derived3() {
    this.z = 4    // slowpath
  }
  let p1 = new Base()
  let p2 = new Base()

  Derived1.prototype = p1
  Derived2.prototype = p1
  Derived3.prototype = p2

  Derived1.prototype.sub = "sub"  // store: Derived1.prototype.sub = "sub" --> type: MONO_STORE_PROPERTY

  p1.w = 5                        // store: p1.w = 5 --> type: STORE_PROPERTY
  let obj1 = new Derived1()
  let obj2 = new Derived2()
  let obj3 = new Derived3()
  print(obj1.w)         // load: obj1.w --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(obj1.x)         // load: obj1.x --> type: LOAD_PROPERTY
  print(obj1.sub)       // load: obj1.sub --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(obj2.w)         // load: obj2.w --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(obj2.y)         // load: obj2.y --> type: LOAD_PROPERTY
  print(obj2.sub)       // load: obj2.sub --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(obj3.w)         // slowpath
  print(obj3.z)         // slowpath
  print(obj3.sub)       // slowpath
}
Test3();
ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.printTypedOpProfiler("STORE_PROPERTY");
ArkTools.printTypedOpProfiler("MONO_LOAD_PROPERTY_ON_PROTO");
ArkTools.clearTypedOpProfiler();
print(ArkTools.isAOTDeoptimized(Test3));
print("======================PART4=======================")
function Test4() {
  function Level0() {
    this.w = 1    // slowpath: not dumped due to protochanged
  }
  function Level1() {
    this.x = 2    // slowpath: not dumped due to protochanged
  }
  function Level2() {
    this.y = 3    // store: this.y --> type: MONO_STORE_PROPERTY
  }

  let p0 = new Level0()
  Level1.prototype = p0
  let p1 = new Level1()
  Level2.prototype = p1
  Level0.prototype.add = () => { print("add") } // store: Level0.prototype.add --> type: MONO_STORE_PROPERTY
  Level1.prototype.sub = () => { print("sub") } // store: Level1.prototype.sub --> type: MONO_STORE_PROPERTY
  Level2.prototype.mul = () => { print("mul") } // store: Level2.prototype.mul --> type: MONO_STORE_PROPERTY

  let obj = new Level2()
  print(obj.w)  // load: obj.w --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(obj.x)  // load: obj.x --> type: MONO_LOAD_PROPERTY_ON_PROTO
  print(obj.y)  // load: obj.y --> type: LOAD_PROPERTY
  obj.add()     // load: obj.add --> type: MONO_LOAD_PROPERTY_ON_PROTO
  obj.sub()     // load: obj.sub --> type: MONO_LOAD_PROPERTY_ON_PROTO
  obj.mul()     // load: obj.mul --> type: MONO_LOAD_PROPERTY_ON_PROTO
}
Test4()
ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.printTypedOpProfiler("MONO_LOAD_PROPERTY_ON_PROTO");
ArkTools.clearTypedOpProfiler();
print(ArkTools.isAOTDeoptimized(Test4));
print("======================PART5=======================")
function Test5() {
  function Foo() {
    this.x = 1    // store: this.x --> type: MONO_STORE_PROPERTY
  }
  function Bar() {
    this.y = 2    // store: this.y --> type: MONO_STORE_PROPERTY
  }
  let p = Foo.prototype
  Bar.prototype = p
  let obj1 = new Foo()
  let obj2 = new Bar()
  print(obj1.x)   // load: obj1.x --> type: LOAD_PROPERTY
  print(obj2.x)   // slowpath
  print(obj2.y)   // load: obj2.y --> type: LOAD_PROPERTY
}
Test5()
ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
print(ArkTools.isAOTDeoptimized(Test5));
print("======================PART6=======================")
function Test6() {
  let p1 = {x: 1}
  let p2 = {x: 1, y: 1, a: 1, b: 1, c: 1}
  let p3 = {x: 2, y: 2}
  let protoArr = [p1, p2, p3]
  for (let i = 0; i < 3; i++) {
      function Foo() {
          this.z = 3
      }
      Foo.prototype = protoArr[i]
      Foo.prototype.m = {}
      let foo = new Foo()
      print(foo.x)
      print(foo.y)
      print(foo.z)
  }
}
Test6()
print("======================PART7=======================")
function Test7() {
  let proto = {}
  for (let i = 0; i < 2; i++) {
      function foo() {
          this.x = 1
      }
      foo.prototype = proto
      let obj = new foo()
      print(obj.x)
  }
}
Test7()
print("======================PART8=======================")
function Test8() {
  let proto = {}
  for (let i = 0; i < 2; i++) {
      function foo() {
          this.x = 1
      }
      foo.prototype = proto
      let obj = new foo()
      print(obj.x)
      print(obj.y)
      proto.y = 2
  }
}
Test8()
print("======================PART9=======================")
function Test9() {
  function Base() {
    this.w = 1    // store: this.x --> type: MONO_STORE_PROPERTY * 2
  }
  Base.prototype.add = () => { print("add") } // store: Base.prototype.add --> type: MONO_STORE_PROPERTY
  function Derived1() {
    this.x = 2    // deopt
  }
  function Derived2() {
    this.y = 3    // deopt
  }
  let p1 = new Base()
  let p2 = new Base()

  Derived1.prototype = p1
  p1.u = 1                 // deopt
  p1.v = 2
  Derived2.prototype = p1

  let obj1 = new Derived1()
  let obj2 = new Derived2()
  print(obj1.w)
  print(obj1.x)
  print(obj1.u)
  print(obj2.w)
  print(obj2.y)
  print(obj2.u)
}
Test9();
ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
ArkTools.clearTypedOpProfiler();
print(ArkTools.isAOTDeoptimized(Test9));
