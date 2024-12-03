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

/*
 * @tc.name:throwerror
 * @tc.desc:test throwerror
 * @tc.type: FUNC
 */
function f1(a2, a3) {
    g = 256;
}
try {
    ([1,2]).filter(f1);
} catch (e) {
    print(e.name);
}

function f0(a1) {
    return a1 in "valueOf";
}
const v4 = [4294967297];
try {
    ([1,2]).reduce(f0, v4);
} catch (e) {
    print(e.name);
}

function f2(a1) {
    return f2;
}
class C2 extends f2 {
}
C2.__proto__ = [8069];
try {
    new C2();
} catch (e) {
    print(e.name);
}

class C3 {
    #a = "2F";
}
try {
    C3(..."g2");
} catch (e) {
    print(e.name);
}

try {
    const v40 = [1, -3.0];
    v40[1] = v40;
    print(v40.flat(536870887));
} catch (e) {
    print(e)
}
