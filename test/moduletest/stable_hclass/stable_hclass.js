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

class A{}
class B extends A{}
class C extends B{}

let cnt = 0;

function F()
{
    let c = new C();
    c.x = 1;
    
    let c2 = new C();
    c2.x = 1;
    
    c.y = 1;


    assert_equal(ArkTools.isStableHClass(c2), false);
    c2.z = 1;
    assert_equal(ArkTools.isStableHClass(c2), true);

    for (let i = 1; i <= 1021; i++) {
        c2[`x${i}`] = 1;
        assert_equal(ArkTools.isStableHClass(c2), true);
    }

    c2['x1023'] = 1;
    assert_equal(ArkTools.isStableHClass(c2), false);


    A.prototype.o = 1;
    assert_equal(ArkTools.isStableHClass(B.prototype), true);
    for (let i = 1; i <= 2000; i++) {
        B.prototype[`y${i}`] = 1;
    }
    assert_equal(ArkTools.isStableHClass(B.prototype), false);
}

F();

test_end();