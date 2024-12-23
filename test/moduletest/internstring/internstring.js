/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @tc.name: internstring
 * @tc.desc: Test internstring fast check is number or not
 * @tc.type: FUNC
 */
{
    let REGISTRY = Symbol();
    function System() {
      this[REGISTRY] = {};
    }
  
    function SetAndPrintLoad(loader, id) {
      let load = loader[REGISTRY][id];
      print("id: " + id + " - load: "+ load);
      if (loader[REGISTRY][id] == undefined) {
          loader[REGISTRY][id] = id;
      } else {
          loader[REGISTRY][id] = undefined;
      }
    }
  
    let keyArray = ["12345678", "1234567891", "9999999999", "135", "0", "abc", "cba",
                    "12345678", "1234567891", "9999999999", "135", "0", "abc", "cba"];
    
    let system = new System();
    for (let T = 0; T < 2; T++) {
        for (let i = 0; i < keyArray.length; i++) {
            SetAndPrintLoad(system, keyArray[i]);
        }
    }
}