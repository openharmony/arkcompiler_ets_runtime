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
 * @tc.name:getdeletedelementsat
 * @tc.desc:test GetDeletedElementsAt
 * @tc.type: FUNC
 * @tc.require: issueIBFP2E
 */

// This case aims to check return value of GetDeletedElementsAt
{
    let map1 = new Map();
    map1.set(0, 1);
    map1.set(1, 2);
    map1.set(2, 3);
    map1.set(3, 4);
    map1.delete(0);
    map1.forEach(function () { map1.clear() });
    print("Test runs successfully!");
}
