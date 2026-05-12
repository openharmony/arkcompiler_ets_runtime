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

/*enum FILE_MODE {
  FOLDER_MODE = 1,
  FILE_MODE = 2
}*/

class FILE_MODE {
  static  folder = 1;
  static  file = 2;
}

class NormalFileInfo {
  uri;
  relativePath;
  fileName;
  mode;
  size;
  mtime;
  mimeType;
  constructor(uri, relativePath, mode, size, mtime, mimeType)
  {
    this.uri = uri;
    this.relativePath = relativePath;
    this.mode = mode;
    this.size = size;
    this.mtime = mtime;
    this.mimeType = mimeType;
  }
}

class TestFileInfo {

  fileName; // File name and extension
  uri;
  mtime = 0; // seconds
  size = 0; // KB
  mimeType;
  fileIconMax ;
  fileIconMin ; // phone宫格和列表视图下图片大小不同
  mode = 'file';
  relativePath = '';

  addedDate = 0; // seconds
  isShortCut = false;


  id;
  duration = 0;
  subFolderList;
  subFileList;
  layer = 0;
  autoShow = false;
  isExternalStorageDeviceFile = false;
  isChecked = false;
  path;
  sub = 0;
  rootName;
  parentPathForDetail;
  isExist = true;

  constructor(obj) {
    if (obj === undefined) {
      return;
    }
    this.fileName = obj.fileName || '';
    this.uri = obj.uri || '';
    this.mtime = obj.mtime || 0;
    this.size = obj.size || 0;
    this.mimeType = obj.mimeType || '';
    this.relativePath = obj.relativePath || '';
    this.addedDate =  0;
    this.id =  randomStrId();
    this.duration =  0;
    this.isChecked = false;
    this.path = obj.relativePath || '';
    this.sub =  0;
    this.rootName =  '';
    this.parentPathForDetail =  '';
  }
}



function fromNormalFileInfo(fileAccessFileInfo) {
  let fileInfo = new TestFileInfo(fileAccessFileInfo);
  if (fileAccessFileInfo.mode === FILE_MODE.FOLDER_MODE) {
    fileInfo.mode = 'folder';
  } else {
    fileInfo.mode = 'file';
  }
  return fileInfo;
}

function createFileInfo()
{
  let res= [];
  for (let i = 0; i < 10000; i++) {
    let uri = randomStrId();
    let relativePath = randomStrId();
    let mode = i%100?FILE_MODE.FOLDER_MODE:FILE_MODE.FILE_MODE;
    let size = randomId();
    let mtime = randomId();
    let mimeType = randomStrId();
    res.push(new NormalFileInfo(uri, relativePath, mode, size, mtime, mimeType));
  }
  return res;
}

function transferFileInfo(fileAccessFileInfo)
{
  let res = [];
  for (let i = 0; i < 10000; i++) {
    let tmpFileInfo = fromNormalFileInfo(fileAccessFileInfo[i])
    res.push(tmpFileInfo);
  }
  return res;
}



function randomStrId()
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

function randomId()
{
  let res = Math.round(Math.random() * 10000000);
  return res;
}

function testTime(){
  var now = new Date();
  var time = now.getHours() + ":" + now.getMinutes() + ":" + now.getSeconds() + "." + now.getMilliseconds();
  return time;
}

function testFileInfo() {
  let firstTime;
  firstTime = testTime();
  let setupStart = Date.now();
  let normalFile = createFileInfo();
  let endTime = Date.now();

  print('testfileinfo-create: ms = ' + String(endTime - setupStart));

  setupStart = Date.now();
  let testFile = transferFileInfo(normalFile);
  endTime = Date.now();
  print('testfileinfo-transfer: ms = ' + String(endTime - setupStart));

}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  testFileInfo()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

testFileInfo();

