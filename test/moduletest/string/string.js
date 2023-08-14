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
 * @tc.name:String
 * @tc.desc:test String
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
let tmp = '不林肯访华:习近平会晤美国国务卿促“让中美关系稳下来好起来”美国国务卿安东尼于6月1日于北京与习近平会面。大杂烩中国都五年来首位访华\
的美国国务卿王刚说,你吵的菜安倍晋三吃了赞不绝口,但是王毅不喜欢吃,他接到一个快递单号为#nsExpress3SF#123456789的包裹';
let flag = tmp;
let str1 = tmp.substring(0, 111) + ',,,,,,,,,,,,,,,,,,,,,,,' + tmp.substring(111 + 23);
let str2 = tmp.substring(0, 111) + 'nsExpress3SF#123456789的' + tmp.substring(111 + 23,111 + 24) + '需';
let str3 = tmp.substring(0, 111) + 'nsExpress3SF#123456789的' + tmp.substring(111 + 23);
print((flag === str1).toString());
print((flag === str2).toString());
print((flag === str3).toString());

let utf81 = '12312463347659634287446568765344234356798653455678899865746435323456709876543679334675235523463756875153764736256\
235252537585734523759078945623465357867096834523523765868';
let utf82 = utf81.substring(0, 169) + '8';
let utf83 = utf81.substring(0, 169) + '0';
print((utf81 === utf82).toString());
print((utf81 === utf83).toString());

function foo(a) {
    return a;
}
try {
    for (let i = 0; i < 25; i++) {
        foo += foo;
    }
} catch (e) {
    print(e.message);
}