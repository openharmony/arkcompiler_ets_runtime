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

{
    function fn0() { }
    var v1 = fn0;
    for (var v2 = 0; v2 < 2000; ++v2) {
        v1 = v1.bind();
        v1 = v1.bind();
    }
    let v3 = v1.name;
}


{
    // Test 1: Simple bound function
    function foo() { }
    const boundFoo = foo.bind({});
    assert_equal(boundFoo.name, "bound foo");

    // Test 2: Nested bind (bind twice)
    const boundFoo2 = boundFoo.bind({});
    assert_equal(boundFoo2.name, "bound bound foo");

    // Test 3: Triple nested bind
    const boundFoo3 = boundFoo2.bind({});
    assert_equal(boundFoo3.name, "bound bound bound foo");

    // Test 4: Anonymous function
    const anon = function () { }.bind({});
    assert_equal(anon.name, "bound ");

    // Test 5: Arrow function bound
    const arrow = ((a, b) => a + b).bind({});
    assert_equal(arrow.name, "bound ");

    // Test 6: Function with specific name
    function add(a, b) { return a + b; }
    const boundAdd = add.bind(null, 1);
    assert_equal(boundAdd.name, "bound add");

    // Test 7: Method bound
    const obj = {
        methodName: function () { return "test"; }
    };
    const boundMethod = obj.methodName.bind(obj);
    assert_equal(boundMethod.name, "bound methodName");

}

test_end();