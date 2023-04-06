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
 * @tc.name:jsonparser
 * @tc.desc:test Json.parse
 * @tc.type: FUNC
 * @tc.require: issue#I6BFOC
 */

let json = JSON.parse("[ 1, 2, 3]");
print(json);
let json2 = JSON.parse("[       1       ]");
print(json2);
let json3 = JSON.parse("[              ]");
print(json3);
let data = {
    "11111111" : "https://www.a.com",
    "22222222" : "https://www.b.com",
    "00000000" : "https://www.c.com"
}
let strData = JSON.stringify(data);
let res = JSON.parse(strData);
print(res["11111111"]);
print(res["22222222"]);
print(res["00000000"]);