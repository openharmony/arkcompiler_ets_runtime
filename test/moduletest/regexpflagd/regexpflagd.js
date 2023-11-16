/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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
 * @tc.name:regexpflagd
 * @tc.desc:test Regexp d flag
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var regexpNames = /姓氏：(?<first>.+)，名字：(?<last>.+)/gmd;
var users = `姓氏：李，名字：雷\n姓氏：韩，名字：梅梅`;
var result = regexpNames.exec(users);
print(regexpNames.hasIndices);
print(regexpNames.flags);
print(result.indices[0]);
print(result.indices[1]);
print(result.indices[2]);
print(result.indices.groups.first);
print(result.indices.groups.last);
print(result.groups.first);
print(result.groups.last);

result = regexpNames.exec(users);
print(result.indices[0]);
print(result.indices[1]);
print(result.indices[2]);
print(result.indices.groups.first);
print(result.indices.groups.last);
print(result.groups.first);
print(result.groups.last);

var result2 = "bπb".match(/(π)/du).indices;
print(result2[0]);
print(result2[1]);
print(result2.groups);

var regexpFlags = new RegExp("", "dgimsuy").flags;
print(regexpFlags);

print(Reflect.set({}, 4294967284, {}, new Int32Array()))