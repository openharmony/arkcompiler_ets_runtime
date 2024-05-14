/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const { GLFrame } = require('./engine/GLFrame');
const { Scr } = require('./engine/XDefine');
const { XTools } = require('./engine/XTools');
const { XSelect } = require('./engine/control/XSelect');
const { X2DFast } = require('./engine/graphics/X2DFast');
const { IrViewer } = require('./ir/IrViewer');
const { LogParser } = require('./ir/LogParser');

class MainEditor {
  constructor() {
    XTools.LoadConfig();

    this.filePoint_ = '';
    this.files_ = [];
    this.viewer_ = {};
    LogParser.Load('test.txt', this.onLoad.bind(this));

    this.selectFile_ = new XSelect(this.files_, this.filePoint_);
    this.selectFile_.registCallback(this.changeFile.bind(this));

    GLFrame.gi().pCallbackDropfile = this.onDrop.bind(this);
  }
  changeFile(name) {
    this.filePoint_ = name;
  }
  onLoad(fn, result) {
    try {
      let irv = new IrViewer(fn, result);
      if (this.files_.indexOf(fn) < 0) {
        this.files_.push(fn);
        this.selectFile_.resetList(this.files_, fn);
        this.changeFile(fn);
      }
      this.viewer_[fn] = irv;
    }
    catch (e) {
      XTools.PROC_TO = 0;
      console.log(e);
      alert('读取' + fn + '失败');
      return;
    }
  }
  onDrop(files, x, y) {
    if (files.length === 1) {
      let reader = new FileReader();
      reader.readAsDataURL(files[0]);
      reader.onload = (e) => {
        let ret = atob(e.target.result.split(',')[1]);
        this.onLoad(files[0].name, ret);
      };
    }
  }
  static pInstance_ = null;
  static gi() {
    if (MainEditor.pInstance_ === null) {
      MainEditor.pInstance_ = new MainEditor();
    }
    return MainEditor.pInstance_;
  }

  onDraw() {
    if (this.selectFile_.list_.length <= 0) {
      X2DFast.gi().drawText('拖入log文件', 30, Scr.logicw / 2, Scr.logich / 2, 1, 1, 0, -2, -2, 0xff000000);
      return;
    }

    for (let v in this.viewer_) {
      if (this.filePoint_ === v) {
        this.viewer_[v].onDraw();
      }
    }

    this.selectFile_.move(Scr.logicw - 200 - 10, 10, 200, 20);
    this.selectFile_.draw();
    if (XTools.PROC_TO > 0 && XTools.PROC_TO < 100) {
      X2DFast.gi().fillRect(0, Scr.logich - 5, XTools.PROC_TO * Scr.logicw / 100, 5, 0xffff0000);
    }
  }

  onTouch(msg, x, y) {
    if (this.selectFile_.list_.length <= 0) {
      return true;
    }
    if (this.selectFile_.onTouch(msg, x, y)) {
      return true;
    }
    for (let v in this.viewer_) {
      if (this.filePoint_ === v) {
        if (this.viewer_[v].onTouch(msg, x, y)) {
          return true;
        }
      }
    }
    return false;
  }

  onKey(k) {
    for (let v in this.viewer_) {
      if (this.filePoint_ === v) {
        if (this.viewer_[v].onKey(k)) {
          return true;
        }
      }
    }
    return true;
  }
}

module.exports = {
  MainEditor
};