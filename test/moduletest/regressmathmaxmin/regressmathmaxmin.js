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
// mjsunit/regress/regress-2444.js
var flags;

function resetFlags(size) {
    flags = Array(size);
    while (size--) flags[size] = 0;
}

function assertFlags(array) {
    if (flags.length != array.length) {
        print("array != flags");
        return false
    } else{
        for (var i = 0; i < flags.length; ++i) {
            if (flags[i] != array[i]) {
                print("array != flags");
                return false
            }
        }
        return true
    }
}

function object_factory(flag_index, value, expected_flags) {
    var obj = {};
    obj.valueOf = function() {
        assertFlags(expected_flags);
        flags[flag_index]++;
        return value;
    }
    return obj;
}

// assertEquals(-Infinity, Math.max());
print(Math.max());

resetFlags(1);
// assertEquals(NaN,
print(Math.max(object_factory(0, NaN, [0])));
assertFlags([1]);

resetFlags(2);
// assertEquals(NaN,
print(Math.max(object_factory(0, NaN, [0, 0]),
               object_factory(1,   0, [1, 0])));
assertFlags([1, 1]);

resetFlags(3);
// assertEquals(NaN,
print(Math.max(object_factory(0, NaN, [0, 0, 0]),
               object_factory(1,   0, [1, 0, 0]),
               object_factory(2,   1, [1, 1, 0])));
assertFlags([1, 1, 1]);

resetFlags(3);
// assertEquals(NaN,
print(Math.max(object_factory(0,   2, [0, 0, 0]),
               object_factory(1,   0, [1, 0, 0]),
               object_factory(2, NaN, [1, 1, 0])));
assertFlags([1, 1, 1]);

resetFlags(3);
// assertEquals(2,
print(Math.max(object_factory(0,   2, [0, 0, 0]),
               object_factory(1,   0, [1, 0, 0]),
               object_factory(2,   1, [1, 1, 0])));
assertFlags([1, 1, 1]);

// assertEquals(+Infinity, Math.min());
print(Math.min());

resetFlags(1);
// assertEquals(NaN,
print(Math.min(object_factory(0, NaN, [0])));
assertFlags([1]);

resetFlags(2);
// assertEquals(NaN,
print(Math.min(object_factory(0, NaN, [0, 0]),
               object_factory(1,   0, [1, 0])));
assertFlags([1, 1]);

resetFlags(3);
// assertEquals(NaN,
print(Math.min(object_factory(0, NaN, [0, 0, 0]),
               object_factory(1,   0, [1, 0, 0]),
               object_factory(2,   1, [1, 1, 0])));
assertFlags([1, 1, 1]);

resetFlags(3);
// assertEquals(NaN,
print(Math.min(object_factory(0,   2, [0, 0, 0]),
               object_factory(1,   0, [1, 0, 0]),
               object_factory(2, NaN, [1, 1, 0])));
assertFlags([1, 1, 1]);

resetFlags(3);
// assertEquals(0,
print(Math.min(object_factory(0,   2, [0, 0, 0]),
               object_factory(1,   0, [1, 0, 0]),
               object_factory(2,   1, [1, 1, 0])));
assertFlags([1, 1, 1]);

print(Math.max(2, 465, 30.4302, -34.444));
print(Math.max(2, 465, undefined, -34.444));
print(Math.max(2, 465, undefined, new Date()));

print(Math.min(2, 465, 30.4302, -34.444));
print(Math.min(2, 465, undefined, -34.444));
print(Math.min(2, 465, undefined, new Date()));