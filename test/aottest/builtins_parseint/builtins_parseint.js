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

function randomStrId()
{
  let str = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
  let res = '';
  for (let i = 0; i < 13; i++) {
    let n = parseInt(Math.random() * str.length + '');
    res += str[n];
  }
  return res;
}

function testFileInfo() {
  let res = [];
  for (let i = 0; i < 1; i++) {
    let uri = randomStrId();
    res.push(uri);
  }
  return res
}

print(testFileInfo().length)
print(parseInt("0X4"))
print(parseInt("0x4"))
print(parseInt("0X123"))
print(parseInt("0x123"))
