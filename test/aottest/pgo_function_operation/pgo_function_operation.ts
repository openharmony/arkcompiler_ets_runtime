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
declare var ArkTools:any;

function Test1() {
  function A() {
    this.x = "1"; // store: this.x --> type: MONO_STORE_PROPERTY
  }

  // store: A.prototype.foo --> type: MONO_STORE_PROPERTY
  A.prototype.foo = function() {
    print("foo");
  }

  // store: A.prototype.bar --> type: MONO_STORE_PROPERTY
  A.prototype.bar = function() {
    print("bar");
  }

  let a = new A();
  // load: a.x --> type: LOAD_PROPERTY
  print(a.x);
  // store: a.x --> type: STORE_PROPERTY
  a.x = "2";
  // load: a.x --> type: LOAD_PROPERTY
  print(a.x);
  // load: a.foo --> type: MONO_LOAD_PROPERTY_ON_PROTO
  a.foo();
  // load: a.bar --> type: MONO_LOAD_PROPERTY_ON_PROTO
  a.bar();
}
Test1();
ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.printTypedOpProfiler("STORE_PROPERTY");
ArkTools.printTypedOpProfiler("MONO_LOAD_PROPERTY_ON_PROTO");
ArkTools.clearTypedOpProfiler();
print(ArkTools.isAOTDeoptimized(Test1));

function Test2() {
  function B() { // asm
    this.x = 1;
  }

  function foo(b) {
    print(b.bar); // --> slow path
  }

  let b = new B();
  foo(b);

  // store: B.prototype.bar --> type: MONO_STORE_PROPERTY
  B.prototype.bar = "bar";

  foo(b);
}
Test2();
ArkTools.printTypedOpProfiler("MONO_STORE_PROPERTY");
ArkTools.clearTypedOpProfiler();
print(ArkTools.isAOTDeoptimized(Test2));

//make sure ihc in one thread can only be used once
function Test3() {
  for(let i = 0; i<2; ++i) {
    function A() {
      this.x = 1;
    }
    let a = new A();
    print(a.x); // type: LOAD_PROPERTY, second time will cause deopt
  }
}
Test3();
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.clearTypedOpProfiler();
print(ArkTools.isAOTDeoptimized(Test3));
