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
 * @tc.name:arrayjoin
 * @tc.desc:test Array.join
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
let arr = [1, 2, 3, 4];
arr.fill(42, { toString() { arr.length = 0; } });
print(arr.length);
print(arr);

let rawProto = Number.prototype.__proto__;
Number.prototype.__proto__ = ["tr"];
let v1 = 1.23;
v1.fill(7);
Number.prototype.__proto__ = rawProto
print("fill Number Obj Success!")

var log = [];
var fake =
    {
      get source() {
        log.push("p");
        return {
          toString: function() {
            log.push("ps");
            return "pattern";
          }
        };
      },
      get flags() {
        log.push("f");
        return {
          toString: function() {
            log.push("fs");
            return "flags";
          }
        };
      }
    }
RegExp.prototype.toString.call(fake);
print(JSON.stringify(["p", "ps", "f", "fs"]) == JSON.stringify(log));
