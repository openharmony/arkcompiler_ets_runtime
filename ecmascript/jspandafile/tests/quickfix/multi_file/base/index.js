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
import { nop, bar } from './module.js';

function foo() {
  print('base foo()');
}

function getBar() {
    print('base bar');
    return bar;
}

class A {
  constructor(a) {
    this.a = a;
  }

  onCreate() {
    print(this.a);
    print('base A onCreate');
  }

  get message() {
    return 'base helloworld';
  }

  set message(newValue) {
    this.a = 'base helloworld!';
  }

  render() {
    print('base A render ');
  }
}


class A1 {}
class A2 {}
class A3 {}
class A4 {}
class A5 {}
class A6 {}
class A7 {}
class A8 {}
class A9 {}

class A11 {}
class A21 {}
class A31 {}
class A41 {}
class A51 {}
class A61 {}
class A71 {}
class A81 {}
class A91 {}

class A12 {}
class A22 {}
class A32 {}
class A42 {}
class A52 {}
class A62 {}
class A72 {}
class A82 {}
class A92 {}

class A13 {}
class A23 {}
class A33 {}
class A43 {}
class A53 {}
class A63 {}
class A73 {}
class A83 {}
class A93 {}


class A14 {}
class A24 {}
class A34 {}
class A44 {}
class A54 {}
class A64 {}
class A74 {}
class A84 {}
class A94 {}
