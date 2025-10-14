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
 * @tc.name:array
 * @tc.desc:test cowarray
 * @tc.type: FUNC
 * @tc.require: issueIBST79
 */


for (let i = 0; i < 2; i++) {
    let arr = [0, 0]
    assert_equal(JSON.stringify(arr),"[0,0]");
    delete arr[0];
}

for (let i = 0; i < 2; i++) {
    let arr = [0, 0]
    assert_equal(JSON.stringify(arr),"[0,0]");
    arr[0]=1;
}

for (let i = 0; i < 2; i++) {
    let arr = [0, 0]
    assert_equal(JSON.stringify(arr),"[0,0]");
    arr.length = 3;
}

// This use case is used to check whether the built-in interfaces of Array performs a COWArray check when modifying
// the array itself.
{
    function createNewArray() {
        return ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"];
    }

    let oriNumList = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"];
    let numList = createNewArray();

    numList.sort((a, b) => b - a);
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');

    numList.push(11);
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');

    numList.pop();
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');

    numList.shift();
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');

    numList.unshift(0);
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');

    numList.splice(1, 1);
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');

    numList.reverse();
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');

    numList.fill(0, 0, 9);
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');

    numList.copyWithin(0, 1, 3);
    numList = createNewArray();
    assert_equal(JSON.stringify(numList), '["1","2","3","4","5","6","7","8","9","10"]');
}

test_end();
