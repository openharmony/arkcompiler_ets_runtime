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

class Body {
  constructor(x, y) {
    this.x = x;
    this.y = y;
  }
}

class Foot extends Body {
  constructor(x, y, u, v) {
    super(x, y);
    this.u = u;
    this.v = v;
  }
}

class Arm extends Body {
  constructor(x, y, t) {
    super(x, y);
    this.t = t;
  }
}

function foo(p)
{
  return p.x;
}

let a = new Body(1, 23);
let b = new Foot(3, 3.2, 2, 32.3);
let c = new Arm(1.3, 23.2, 23);

for (let i = 0; i < 1000000; i++) {
  foo(a);
  foo(b);
  foo(c);
}
