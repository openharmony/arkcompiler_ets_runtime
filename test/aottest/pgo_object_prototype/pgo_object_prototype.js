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

var proto = {
    method(x) {
        return -1;
    }
}

var object = {
    method(x) {
        return x;
    }
}

Object.setPrototypeOf(object, proto);
// Check usage of proto's HClass after AOT compile
print(proto.method(50));
print(object.method(50));

function F0() {
    this.f0 = 0;
}
let v3 = new F0();
let v4 = new F0();
class C27 { 
    c270 = 0;
}
class C28 { 
    c270 = 0;
}
class C42 extends C27 {
}


C27.__proto__ = v4;

v3.__proto__ = C27;

Object.defineProperty(C27, "d", { configurable: true, value: 1 });          
print("success")