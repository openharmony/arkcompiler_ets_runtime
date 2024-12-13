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

class A {
  a = 0;
  constructor() {
    this.a = 1;
  };
}

let a = new A();

class B {
  b = 0;
  constructor() {
    this.b = 1;
  };
}
let b = new B();

class C {
  c = 0;
  constructor() {
    this.c = 1;
  };
}
let c = new C();

class D {
  d = 0;
  constructor() {
    this.d = 1;
  };
}
let d = new D();

class E {
  e = 0;
  constructor() {
    this.e = 1;
  };
}
let e = new E();

class F {
  f = 0;
  constructor() {
    this.f = 1;
  };
}
let f = new F();

class G {
  g = 0;
  constructor() {
    this.g = 1;
  };
}
let g = new G();

class H {
  h = 0;
  constructor() {
    this.h = 1;
  };
}
let h = new H();

class I {
  i = 0;
  constructor() {
    this.i = 1;
  };
  foo = function (x) {
    this.i = x;
    this.bar(x);
  };
  bar = function (x) {
    this.i = x;
    this.baz(x);
  };
  baz = function (x) {
    this.i = x;
  };
}
let i = new I();
print('hello');
