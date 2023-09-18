# Copyright (c) 2023 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

declare function print(arg:any) : string;
declare interface ArkTools {
    timeInUs(arg:any):number
}

function testToString() {
    let theDog = new Dog("Gabby", "Lab");
    let res;
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        res = theDog.toString();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print(res);
    print("Object ToString:\t" + String(time) + "\tms");
}

testToString();
