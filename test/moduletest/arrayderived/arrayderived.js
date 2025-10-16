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
/*
 * @tc.name: arrayderived
 * @tc.desc: test Array derived and prototype changed
 * @tc.type: FUNC
 */
class CustomArray extends Array {}

// 测试受 `constructor` 属性影响的方法
function testConstructorEffect() {
    let custom = new CustomArray(1, 2, 3);

    // Array.prototype.concat
    let result = custom.concat([4, 5]);
    assert_equal(result instanceof CustomArray, true);

    // Array.prototype.map
    result = custom.map(x => x * 2);
    assert_equal(result instanceof CustomArray, true);

    // Array.prototype.filter
    result = custom.filter(x => x > 1);
    assert_equal(result instanceof CustomArray, true);

    // Array.prototype.slice
    result = custom.slice(1);
    assert_equal(result instanceof CustomArray, true);

    // Array.prototype.flatMap
    result = custom.flatMap(x => [x, x * 2]);
    assert_equal(result instanceof CustomArray, true);

    // Array.prototype.flat
    result = custom.flat();
    assert_equal(result instanceof CustomArray, true);
}

function testArrayprototypechange() {
    let newProto = {};
    Object.defineProperty(newProto, '1', {
        get: function() {
            return 1;
        }
    })
    Array.prototype.__proto__ = newProto;
    let a = new Array(10);
    assert_equal(a[1], 1);
}

testConstructorEffect()

testArrayprototypechange()

test_end();