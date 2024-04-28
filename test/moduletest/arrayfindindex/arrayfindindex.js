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
 * @tc.name:arrayfindIndex
 * @tc.desc:test Array.findIndex
 * @tc.type: FUNC
 * @tc.require: issueI8D8UH
 */

(function(){
    var a = [0, 1,,3];
    var index = a.findIndex(function(val){
        return val === undefined;
    });
    print(index);
})();

(function () {
    var array1 = [,];

    function findIndex() {
      return array1.findIndex(v => v > 0);
    }
    array1.__proto__.push(6);
    print(findIndex())
})();

{
    class C1 {
        [28262n] = 28262n;
    }
    const v2 = new C1();
    const v3 = [-4.869758437495864e+307,1000000000000.0];
    function f4(a5, a6) {
        for (const v7 in v2) {
        }
        a6();
        return a6;
    }
    try { v3.findIndex(f4);} catch (err) { print(err)};
}