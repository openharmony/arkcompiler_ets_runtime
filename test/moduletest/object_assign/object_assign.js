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

/*
 * @tc.name: object_assign
 * @tc.desc: test builtin Object.assign
 * @tc.type: FUNC
 * @tc.require: issue11646
 */

// Testcase1: base function of Object.assign.
{
    const target = { a: 1 };
    const source = { b: 2, c: 3 };
    const result = Object.assign(target, source);
    assert_equal(JSON.stringify(target), '{"a":1,"b":2,"c":3}');
    assert_equal(result, target);
}
// Testcase2: multiple source objects, the properties of the latter objects will overwrite those of the former ones.
{
    const target = { a: 1 };
    const source1 = { b: 2, c: 3 };
    const source2 = { c: 4, d: 5 };
    Object.assign(target, source1, source2);
    assert_equal(JSON.stringify(target), '{"a":1,"b":2,"c":4,"d":5}');
}
// Testcase3: copy properties of the Symbol type (enumerable Symbol properties will be copied).
{
    const sym = Symbol('test');
    const target = {};
    const source = { [sym]: 'symbol value', a: 1 };
    Object.assign(target, source);
    assert_equal(target[sym], "symbol value");
    assert_equal(target.a, 1);
}
// Testcase4: ignore non-enumerable properties and inherited properties.
{
    const target = {};
    const source = Object.create(
    {inherited: "I am an inherited attribute"},
    {
        enumerableProp: { value: "enumerable", enumerable: true },
        nonEnumerableProp: { value: "unenumerable", enumerable: false }
    }
    );
    Object.assign(target, source);
    assert_equal(JSON.stringify(target), '{"enumerableProp":"enumerable"}');
}
// Testcase5: when the target object is a primitive value, it will be automatically converted to an object.
{
    const result = Object.assign(123, { a: 1 });
    assert_equal(JSON.stringify(result), "123");
    assert_equal(result.a, 1);
}
// Testcase6: if the source object is null or undefined, it will be ignored.
{
    const target = { a: 1 };
    Object.assign(target, null, undefined, { b: 2 });
    assert_equal(JSON.stringify(target), '{"a":1,"b":2}');
}
// Testcase7: handling accessor properties (getters): The getter will be executed and its return value will be
// assigned as a regular property.
{
    const source = {
        get foo() {
            return 'bar';
        }
    };
    const target = {};

    Object.assign(target, source);
    assert_equal(target.foo, 'bar');
}
// Testcase8: processing of arrays (arrays are also objects, and indices are treated as properties).
{
    const target = [1, 2, 3];
    const source = [4, 5];
    Object.assign(target, source);
    assert_equal(target, [4, 5, 3]);
}
// Testcase9: handling of non-writable properties, when the target object has a non-writable property, a property
// with the same name in the source object will cause a TypeError.
{
    const target = Object.defineProperty({}, 'a', {
        value: 1,
        writable: false
    });
    try {
        Object.assign(target, { a: 2 });
    } catch (e) {
        assert_equal(e instanceof TypeError, true); // true（in strict mode）
    }
}
// Testcase10: handling cases while properties were store in elements field and elemnts field is in dictionary mode.
{
    let obj = Object.assign({"11785313": "spt", "35961926": "xel" }, { "139": "sl", "11420": "ls"});
    assert_equal(JSON.stringify(obj), '{"139":"sl","11420":"ls","11785313":"spt","35961926":"xel"}');
}
// Testcase11: handling cases while properties were store in elements field and elemnts field is not in dictionary mode.
{
    let obj = Object.assign({"0": "spt", "1": "xel" }, {"2": "sl", "3": "ls"});
    assert_equal(JSON.stringify(obj), '{"0":"spt","1":"xel","2":"sl","3":"ls"}');
}

{
    function fn0(v0, v1) {}
    {
        let v12 = {
            set 6000(v15) {}
        };
        let v13 = Object.assign({}, v12);
        fn0(Object.values(v13));
    }
}

test_end();
