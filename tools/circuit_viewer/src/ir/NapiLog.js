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

class NapiLog {
  constructor() {}
}
NapiLog.LEV_NONE = 0;
NapiLog.LEV_ERROR = 1;
NapiLog.LEV_DEBUG = 2;
NapiLog.LEV_INFO = 3;

const LEV_STR = ['[NON]', '[ERR]', '[DBG]', '[INF]'];
let logLevel = NapiLog.LEV_INFO;
let logFileName = null;
let logResultMessage = [true, ''];
let errorCallBack = null;

function getDateString() {
  let nowDate = new Date();
  return nowDate.toLocaleString();
}

function saveLog(dateStr, levStr, detail) {
  if (logFileName) {
    let logStr = dateStr + ' ' + levStr + ' ' + detail + '\n';
  }
}

NapiLog.init = function (level, fileName) {
  logLevel =
    level in
    [NapiLog.LEV_NONE, NapiLog.LEV_ERROR, NapiLog.LEV_DEBUG, NapiLog.LEV_INFO]
      ? level
      : NapiLog.LEV_ERROR;
  logFileName = fileName ? fileName : 'napi_generator.log';
};

function recordLog(lev, ...args) {
  let dataStr = getDateString();
  let detail = args.join(' ');
  saveLog(dataStr, LEV_STR[lev], detail);
  if (lev == NapiLog.LEV_ERROR) {
    logResultMessage = [false, detail];
    if (errorCallBack != null) errorCallBack(detail);
  }
  if (logLevel < lev) return;
  console.log(dataStr, LEV_STR[lev], detail);
}

NapiLog.logError = function (...args) {
  recordLog(NapiLog.LEV_ERROR, args);
};

NapiLog.logDebug = function (...args) {
  recordLog(NapiLog.LEV_DEBUG, args);
};

NapiLog.logInfo = function (...args) {
  recordLog(NapiLog.LEV_INFO, args);
};

NapiLog.getResult = function () {
  return logResultMessage;
};

NapiLog.clearError = function () {
  logResultMessage = [true, ''];
};

NapiLog.registError = function (func) {
  errorCallBack = func;
};

module.exports = {
  NapiLog,
};
