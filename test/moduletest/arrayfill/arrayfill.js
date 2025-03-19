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

// Array Fill IR test cases 
function arrayInit(array, value) {
    for (let i = 0; i < array.length; i++) array[i] = value;
}
{
    let array1 = new Array(8);
    // relativeStart < 0, let k be max((len + relativeStart)
    arrayInit(array1, 1);
    array1.fill(-1, -1);
    print(array1);
    arrayInit(array1, 1);
    array1.fill(-1, -10);
    print(array1);
    // If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    arrayInit(array1, 1);
    array1.fill(-1, 0, -1);
    print(array1);

    arrayInit(array1, 1);
    array1.fill(-1, 0, -10);
    print(array1);

    // startArg exceed int32
    arrayInit(array1, 1);
    array1.fill(-1, 2^32 + 1);
    print(array1);

    arrayInit(array1, 1);
    array1.fill(-1,0, - 2^32 - 1);
    print(array1);

    // endArg exceed int32
    arrayInit(array1, 1);
    array1.fill(-1, 0, 2^32 + 1);
    print(array1);

    arrayInit(array1, 1);
    array1.fill(-1, 0, - 2^32 - 1);
    print(array1);

    // startArg and endArg exceed int32
    arrayInit(array1, 1);
    array1.fill(-1, - 2^32 - 1, 2^32 + 1);
    print(array1);

    arrayInit(array1, 1);
    array1.fill(-1, 2^32 + 1, - 2^32 - 1);
    print(array1);

    // string number startArg
    arrayInit(array1, 1);
    array1.fill(-1, "-1");
    print(array1);

    // string number endArg
    arrayInit(array1, 1);
    array1.fill(-1, 0,"-1");
    print(array1);

    // string  startArg
    arrayInit(array1, 1);
    array1.fill(-1, "abcdefg");
    print(array1);

    // string endArg
    arrayInit(array1, 1);
    array1.fill(-1, 0,"abcdefg");
    print(array1);

    // null  startArg
    arrayInit(array1, 1);
    array1.fill(-1, null);
    print(array1);

    // null endArg
    arrayInit(array1, 1);
    array1.fill(-1,0, null);
    print(array1);

    // undefined  startArg
    arrayInit(array1, 1);
    array1.fill(-1, undefined);
    print(array1);

    // undefined endArg
    arrayInit(array1, 1);
    array1.fill(-1,0, undefined);
    print(array1);

    try {
        // startArg int convert exception
        arrayInit(array1, 1);
        array1.fill(-1, 1n);
    } catch (e) {
        print(e.message);
    }

    try {
        // endArg int convert exception
        arrayInit(array1, 1);
        array1.fill(-1, 0, 1n);
    } catch (e) {
        print(e.message);
    }

    try {
        // length int convert exception
        arrayInit(array1, 1);
        const proxyArray = new Proxy(array1, {
            get(target, prop) {
                if (prop === "length") {
                    return 0n; 
                }
                return target[prop];
            }
        });
        proxyArray.fill(-1, 0, 1);
    } catch (e) {
        print(e.message);
    }

}
{
  print([0, 0, 0, 0, 0].fill());
  print([0, 0, 0, 0, 0].fill(8));
  print([0, 0, 0, 0, 0].fill(8, 1));
  print([0, 0, 0, 0, 0].fill(8, 10));
  print([0, 0, 0, 0, 0].fill(8, -5));
  print([0, 0, 0, 0, 0].fill(8, 1, 4));
  print([0, 0, 0, 0, 0].fill(8, 1, -1));
  print([0, 0, 0, 0, 0].fill(8, 1, 42));
  print([0, 0, 0, 0, 0].fill(8, -3, 42));
  print([0, 0, 0, 0, 0].fill(8, -3, 4));
  print([0, 0, 0, 0, 0].fill(8, -2, -1));
  print([0, 0, 0, 0, 0].fill(8, -1, -3));
  print([0, 0, 0, 0, 0].fill(8, undefined, 4));
  print([ ,  ,  ,  , 0].fill(8, 1, 3));
}

function test() {
  var t = [1,2,3];
  function f() {
    var h = [];
    var a = [...arguments]
    for(item in a) {
      var n = new Number(a[item]);
      if (n < 0) {
        n = n + 0x100000;
      }
      h.push(n.toString(16))
    }
    return h;
  }
  var q = f;
  t.length = 20;
  var o = {};
  Object.defineProperty(o,'3', {
    get:function() {
      var ta = [];
      ta.fill.call(t, "nataile")
      return 5;
    }
  })
  t.__proto__ = o;
  var j = [];
  f.apply(null, t).toString()
}
try {test()} catch(err) {print(err)};