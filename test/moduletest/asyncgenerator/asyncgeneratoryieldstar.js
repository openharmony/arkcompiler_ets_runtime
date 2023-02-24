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
 * @tc.name:asyncgenerator
 * @tc.desc:test asyncgenerator function yield star
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

print("asyncgenerator yield star start");
var reason = {};
var obj = {
  get [Symbol.iterator]() {
    throw new Error('it should not get Symbol.iterator');
  },
  [Symbol.asyncIterator]() {
    return {
      next() {
        return {
          then() {
            throw reason;
          }
        };
      }
    };
  }
};



var callCount = 0;

var gen = async function *() {
  callCount += 1;
  yield* obj;
    throw new Error('abrupt completion closes iter');

};

var iter = gen();

iter.next().then(() => {
  throw new Error('Promise incorrectly fulfilled.');
}, v => {
  print(v==reason);
  iter.next().then(({ done, value }) => {
    print(done);
    print(value);
    print("asyncgenerator yield star end");
  });
});

print(callCount);
