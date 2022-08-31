/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

import  * as animal from  './animals.js';

let list = ["Panda", "Lion ", "Monkey"];
function printName(obj, index)
{
    let name = obj[list[index]];
    return name;
}

for(let i = 0; i< 500; ++i) {
    for (let j = 0; j< 3; ++j) {
         printName(animal, j);
    }
}

print("success");
