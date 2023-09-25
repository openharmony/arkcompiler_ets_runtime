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
 * @tc.name:arrayprotochange
 * @tc.desc:test Array function when prototype changes
 * @tc.type: FUNC
 * @tc.require: issueI7O616
 */

var arr = [0, 1, 2, 3, 4];
var arr2 = [0, , , 3, 4];
arr2.__proto__ = arr;
print(arr2[1]);
print(arr2.at(1));