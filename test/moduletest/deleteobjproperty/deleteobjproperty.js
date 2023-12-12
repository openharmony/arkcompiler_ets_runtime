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

let a = {jj:"kaifa", wo:"tian"};
Object.defineProperty(a, 'myname', {
    configurable:false,
    value:222
});
var str = 'j' + 'j';
var b = undefined;
var l = new ArrayBuffer(400);
var v1 = new Int32Array(l);
var Stack = ArkPrivate.Load(ArkPrivate.Stack);
var stack = new Stack();
delete a[str];
try {
    print(delete a.hhh);
} catch (e){
    print(e);
}
try {
    print(delete b.kk);
} catch (e){
    print(e);
}

try {
    var str1 = 'my' + 'name';
    print(delete a[str1]);
} catch (e){
    print(e);
}

try {
    print(delete v1[1000]);
} catch (e){
    print(e);
}

try {
    print(delete stack[1000]);
} catch (e){
    print(e);
}
print(a.myname);
print(a.jj);
