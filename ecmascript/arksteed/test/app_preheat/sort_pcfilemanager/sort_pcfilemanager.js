/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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


class TestGroup {
  // 组内index
  fileName;
  constructor(fileName) {
    this.fileName = fileName;
  }
}
class TestFileOrGroup {
  // 区分当前是文件还是组名
  testIndex;
  // 组内index
  fileName;
  constructor(testIndex, fileName) {
    this.testIndex = testIndex;
    this.fileName = fileName;
  }
}

function randomId()
{
  let str = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
  let res = '';
  for (let i = 0; i < 13; i++) {
    // 随机产生字符串的下标
    let n = parseInt(Math.random() * str.length + '');
    res += str[n];
  }
  return res;
}

function testTime(){
  var now = new Date();
  var time = now.getHours() + ":" + now.getMinutes() + ":" + now.getSeconds() + "." + now.getMilliseconds();
  return time;
}

function sort()
{
  let sortNumber= [];
  for (let i = 0; i < 10000; i++) {
    sortNumber.push(new TestFileOrGroup(Math.floor(Math.random() * 1000), new TestGroup(randomId())));
  }

  let startTime = new Date().getTime();
//  print(firstTime + ' FileSort' + 'sortNumber  start ' + sortNumber.length);
  sortNumber.sort((a, b) => {
    return a.fileName.fileName.localeCompare(b.fileName.fileName)
  });
  let endTime = new Date().getTime();
//  print(secondTime + ' FileSort' + 'sortNumber end');
  print(`sort_pcfilemanager: ms = ${endTime-startTime}`)
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  sort()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

sort();
