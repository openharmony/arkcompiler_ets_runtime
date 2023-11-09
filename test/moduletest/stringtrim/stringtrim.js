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
 * @tc.name:stringSlice
 * @tc.desc:test String.slice
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

let greeting = "                    Hello World!  ";
print(greeting);
print(greeting.trim());

let fooStr = "foo    ";
print(fooStr);
print(fooStr.trim());

let barStr = "    bar";
print(barStr);
print(barStr.trim());

let chinese = "     我，是一个中文句子    我前面有一些空格，我中间有一些空格。     我后面也有一些空格。          ";
print(chinese);
print(chinese.trim());

let a = "a    assseeeeeeeeeeddddddd"
let c = a.substring(1, 20);
let d = c.substring(2, 20);
print(d);
print(d.trim())