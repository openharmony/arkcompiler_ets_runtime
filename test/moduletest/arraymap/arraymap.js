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
 * @tc.name:async
 * @tc.desc:test async function
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G issueI8FBM3
 */
var array = [,];
function map() {
    return array.map(x => x + 1);
}
array.__proto__.push(5);
var narr = map();
print(JSON.stringify(Object.getOwnPropertyDescriptor(narr, 0)));
print(narr[0], 6);

let arr=new Array(10);
map=arr.map(()=>{});
print(map.at(9));

// Species constructor is used to create []
// So not should use ElementAccessor::Set (elements.length == 0)
var instance = [];
var Ctor = function() {
  return instance;
};
var a = [1, 2, 3, 4, 5];
a.constructor = {};
a.constructor[Symbol.species] = Ctor;
var result = a.map(function() {});
print(result == instance);
{
  const v1 = new BigInt64Array();
  const v2 = [65537,-1,4096,-9007199254740990,-268435456,6,-29705,128,-6];
  function f3() {
      v2["pop"]();
      return f3;
  }
  let a = v2.map(f3());
  print(a.length)
}

{
  const v0 = [0,1,2,3,4,5,6,7,8,9,10];
  const mapEd = v0.map(() =>{
    Object.defineProperty(Array.prototype, '8',{value: 42, writable :false });
    return 8;
  })
  print(v0)
}

