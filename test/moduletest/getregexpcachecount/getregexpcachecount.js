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
 * @tc.name: getregexpcachecount
 * @tc.desc: test get regexp cache count 
 * @tc.type: FUNC
 */

var regexpNames = /姓氏：(?<first>.+)，名字：(?<last>.+)/gmd;
var users = `姓氏：李，名字：雷\n姓氏：韩，名字：梅梅`;
var result = regexpNames.exec(users);

assert_equal(result.groups.first, "李");
assert_equal(result.groups.last, "雷");

ArkTools.forceFullGC();
ArkTools.forceFullGC();
ArkTools.forceFullGC();

assert_equal(ArkTools.getRegExpCacheCount(), 0);

test_end();