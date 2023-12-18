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

const { NapiLog } = require('../ir/NapiLog');
export class Scr {
  constructor() {}
  static ReSize(w, h) {
    Scr.width = w;
    Scr.height = h;
    if (Scr.keeplogicworh == 'width') Scr.logich = (Scr.logicw * h) / w;
    else Scr.logicw = (Scr.logich * w) / h;
  }
  static setLogicScreenSize(w, h) {
    if (Scr.logicw == w && Scr.width == w && Scr.logich == h && Scr.height == h)
      return;
    Scr.logicw = w;
    Scr.logich = h;
    Scr.width = w;
    Scr.height = h;

    if ('undefined' != typeof wx) {
      var info = wx.getSystemInfoSync();
      Scr.width = info.windowWidth;
      Scr.height = info.windowHeight;
    }
  }
}

Scr.width = 320;
Scr.height = 240;
Scr.keeplogicworh = 'height';
Scr.logicw = 320;
Scr.logich = 240;
Scr.fps = 60;
