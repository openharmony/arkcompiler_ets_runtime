/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 * @tc.name:linkedhashtable
 * @tc.desc:test linked hash table
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
const map = new Map([[800000005, 100]]);
let obj1 = {"code": 800000005};
let res = JSON.stringify(obj1);
let parseObj = JSON.parse(res);
let flag1 = (parseObj.code === 800000005) &
            map.get(800000005) === map.get(parseObj.code);
let i = Number.parseFloat("800000002");
let obj2 = {i: "aaa", 800000003: "bbbbbb", get a() {}};
let flag2 = (i === 800000002) & (obj2[800000002] === obj2[i])
let flagStr = "";
if (flag1 && flag2) {
    flagStr = "test linkedhashtable success!!!";
} else {
    flagStr = "test linkedhashtable failed!!!";
}
assert_equal(flagStr,"test linkedhashtable success!!!");

var s=new Set([0]);
for(var x of s){
    s.add(x+1);
    if(x>3){
        s.clear();
        s.clear();
    }
}

test_end();
