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
 * @tc.name:compareobjecthclass
 * @tc.desc:test compareHClass
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var res = false;

    let obj1 = {x : 1, y : 2, z :3};
    let obj2 = {y : 2, x : 1, z :3};
    let obj3 = {x : 11, y : 12, z :13};    

if(!ArkTools.compareHClass(obj1, obj2) && ArkTools.compareHClass(obj1, obj3)) {
    res = true;
}

print(res);

print(ArkTools.forceFullGC());

// MemoryReduceDegree::LOW
ArkTools.hintGC(0);
// MemoryReduceDegree::MIDDLE
ArkTools.hintGC(1);
// MemoryReduceDegree::HIGH
ArkTools.hintGC(2);
print("call hintGC to make vm decide to trigger GC or do noting");
