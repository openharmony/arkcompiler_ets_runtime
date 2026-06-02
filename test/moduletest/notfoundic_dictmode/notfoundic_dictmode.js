/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 * @tc.name:notfoundic_dictmode
 * @tc.desc:test not-found IC on dictionary mode object should not return stale result
 * @tc.type: FUNC
 * @tc.require: issueICNDYR
 */

function loadMissingProp(obj) {
    return obj.missing;
}

// Step 1: Create an object and force it into dictionary mode.
let obj = {};
obj.temp = 1;
delete obj.temp;

assert_equal(ArkTools.hasFastProperties(obj), false);

// Step 2: Warm up the IC by repeatedly accessing a non-existent property.
// ProfileTypeInfo is created lazily when the function's hotness counter
// reaches zero. For a small function this requires hundreds of calls.
for (let i = 0; i < 2000; i++) {
    let r = loadMissingProp(obj);
    assert_equal(r, undefined);
}

// Step 3: Add the "missing" property while the object is still in
// dictionary mode. The hclass does NOT change here.
obj.missing = 42;
assert_equal(ArkTools.hasFastProperties(obj), false);

// Step 4: Access the property again.
// The IC should either miss or be invalidated, so the slow path
// finds the property in the dictionary and returns 42.
let result = loadMissingProp(obj);
assert_equal(result, 42);

test_end();
