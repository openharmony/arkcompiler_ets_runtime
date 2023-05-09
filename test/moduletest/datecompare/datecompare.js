/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name:datecompare
 * @tc.desc:test date
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var str1 = Date.parse('08/04/2011')
var str2 = Date.parse('2010-08-04')

var timer1 = new Date(str1)
var timer2 = new Date(str2)

print(timer1 > timer2)