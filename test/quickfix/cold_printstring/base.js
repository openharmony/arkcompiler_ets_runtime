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

// normal function.
function foo() {
  print("base foo")
}

// anonymous function.
function A() {
  (()=>{
      print("anonymous: base A");
  })()
}

// object literal.
function B() {
    var obj_1 = {'a': 1, 'b': "base"}
    var obj = {'a': 1, 'b': obj_1}

    print(JSON.stringify(obj));
}

// array literal.
function C() {
    var arr = ["1", "2", "base"]
    print(arr);
}

globalThis.foo = foo
globalThis.A = A
globalThis.B = B
globalThis.C = C
