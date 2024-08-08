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
 * @tc.name:createobject
 * @tc.desc:test createobject
 * @tc.type: FUNC
 * @tc.require: issueIA955Y
 */
let originObject = {
  0: "lighter",
  1: "normal",
  2: "regular",
  3: "medium",
  4: "bold",
  5: "bolder",
  100: "100",
  200: "200",
  300: "300",
  400: "400",
  500: "500",
  600: "600",
  700: "700",
  800: "800",
  900: "900",
  func1: function(a) {
    print(a);
  },
  func2: function(a, b) {
    print(a + b);
  }
}

Object.keys(originObject).forEach(
  function(key) {
      print(key);
      print(originObject[key]);
  }
);
originObject.func1("wode");
originObject.func2("wode", 5);