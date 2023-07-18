/******/ (() => { // webpackBootstrap
/******/ 	var __webpack_modules__ = ({

/***/ "./src/MainEditor.js":
/*!***************************!*\
  !*** ./src/MainEditor.js ***!
  \***************************/
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

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

const { GLFrame } = __webpack_require__(/*! ./engine/GLFrame */ "./src/engine/GLFrame.js");
const { Scr } = __webpack_require__(/*! ./engine/XDefine */ "./src/engine/XDefine.js");
const { XTools } = __webpack_require__(/*! ./engine/XTools */ "./src/engine/XTools.js");
const { XSelect } = __webpack_require__(/*! ./engine/control/XSelect */ "./src/engine/control/XSelect.js");
const { X2DFast } = __webpack_require__(/*! ./engine/graphics/X2DFast */ "./src/engine/graphics/X2DFast.js");
const { IrViewer } = __webpack_require__(/*! ./ir/IrViewer */ "./src/ir/IrViewer.js");
const { LogParser } = __webpack_require__(/*! ./ir/LogParser */ "./src/ir/LogParser.js");

class MainEditor {
  constructor() {
    XTools.LoadConfig();

    this.filePoint_ = "";
    this.files_ = [];
    this.viewer_ = {}
    LogParser.Load("test.txt", this.onLoad.bind(this));

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
      alert("读取" + fn + "失败");
      return;
    }
  }
  onDrop(files, x, y) {
    if (files.length == 1) {
      let reader = new FileReader();
      reader.readAsDataURL(files[0]);
      reader.onload = (e) => {
        let ret = atob(e.target.result.split(",")[1]);
        this.onLoad(files[0].name, ret);
      }
    }
  }
  static pInstance_ = null;
  static gi() {
    if (MainEditor.pInstance_ == null) MainEditor.pInstance_ = new MainEditor();
    return MainEditor.pInstance_;
  }

  onDraw() {
    if (this.selectFile_.list_.length <= 0) {
      X2DFast.gi().drawText("拖入log文件", 30, Scr.logicw / 2, Scr.logich / 2, 1, 1, 0, -2, -2, 0xff000000);
      return;
    }

    for (let v in this.viewer_) {
      if (this.filePoint_ == v) {
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
      if (this.filePoint_ == v) {
        if (this.viewer_[v].onTouch(msg, x, y)) {
          return true;
        }
      }
    }
    return false;
  }

  onKey(k) {
    // console.log(k);
    for (let v in this.viewer_) {
      if (this.filePoint_ == v) {
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
}

/***/ }),

/***/ "./src/engine/GLFrame.js":
/*!*******************************!*\
  !*** ./src/engine/GLFrame.js ***!
  \*******************************/
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   "gl": () => (/* binding */ gl),
/* harmony export */   "GLFrame": () => (/* binding */ GLFrame)
/* harmony export */ });
/* harmony import */ var _ir_CanvasInput_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ../ir/CanvasInput.js */ "./src/ir/CanvasInput.js");
/* harmony import */ var _ir_CanvasInput_js__WEBPACK_IMPORTED_MODULE_0___default = /*#__PURE__*/__webpack_require__.n(_ir_CanvasInput_js__WEBPACK_IMPORTED_MODULE_0__);
/* harmony import */ var _graphics_X2DFast_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./graphics/X2DFast.js */ "./src/engine/graphics/X2DFast.js");
/* harmony import */ var _XDefine_js__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./XDefine.js */ "./src/engine/XDefine.js");
/* harmony import */ var _XTools_js__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ./XTools.js */ "./src/engine/XTools.js");
/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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






var gl;
var Mouse = {
  MOUSE_LEFT: 0,
  MOUSE_RILLER: 1,
  MOUSE_RIGHT: 2,
};

var MouseEvent = {
  LEFT_CLICK: 1,
  LEFT_MOVE: 2,
  LEFT_RELEASE: 3,
  RIGHT_CLICK: 4,
  RIGHT_MOVE: 5,
  RIGHT_RELEASE: 6,
};

function touchStart(e) {
  document.addEventListener('contextmenu', function (e) {
    e.preventDefault();
  });
  e.preventDefault();
  GLFrame.pinstance_.callbackProctouch(
    MouseEvent.LEFT_CLICK,
    e.touches[0].clientX,
    e.touches[0].clientY
  );
}
function touchMove(e) {
  e.preventDefault();
  GLFrame.pinstance_.callbackProctouch(
    MouseEvent.LEFT_MOVE,
    e.touches[0].clientX,
    e.touches[0].clientY
  );
}
function touchEnd(e) {
  e.preventDefault();
  GLFrame.pinstance_.callbackProctouch(
    MouseEvent.LEFT_RELEASE,
    e.changedTouches[0].clientX,
    e.changedTouches[0].clientY
  );
}

function mouseDown(e) {
  e.preventDefault();
  switch (e.button) {
    case Mouse.MOUSE_LEFT:
      GLFrame.pinstance_.callbackProctouch(
        MouseEvent.LEFT_CLICK,
        e.offsetX,
        e.offsetY
      );
      break;
    case Mouse.MOUSE_RIGHT:
      GLFrame.pinstance_.callbackProctouch(
        MouseEvent.RIGHT_CLICK,
        e.offsetX,
        e.offsetY
      );
      break;
  }
}
function mouseMove(e) {
  e.preventDefault();
  GLFrame.pinstance_.callbackProctouch(
    MouseEvent.LEFT_MOVE,
    e.offsetX,
    e.offsetY
  );
}
function mouseUp(e) {
  e.preventDefault();
  switch (e.button) {
    case Mouse.MOUSE_LEFT:
      GLFrame.pinstance_.callbackProctouch(
        MouseEvent.LEFT_RELEASE,
        e.offsetX,
        e.offsetY
      );
      break;
    case Mouse.MOUSE_RIGHT:
      GLFrame.pinstance_.callbackProctouch(
        MouseEvent.RIGHT_RELEASE,
        e.offsetX,
        e.offsetY
      );
      break;
  }
}
function mouseWheel(e) {
  e.preventDefault();
  if (e.wheelDeltaY > 0) {
    GLFrame.pinstance_.callbackProctouch(10, e.clientX, e.clientY);
  }
  else {
    GLFrame.pinstance_.callbackProctouch(11, e.clientX, e.clientY);
  }
}

function keyUp(e) {
  if (!e.ctrlKey) {
    _XTools_js__WEBPACK_IMPORTED_MODULE_3__.XTools.KEY_CTRL = false;
  }
  if (!e.shiftKey) {
    _XTools_js__WEBPACK_IMPORTED_MODULE_3__.XTools.KEY_SHIFT = false;
  }
  if (!e.altKey) {
    _XTools_js__WEBPACK_IMPORTED_MODULE_3__.XTools.KEY_ALT = false;
  }
  e.preventDefault();
}

function keyDown(e) {
  let ret = '';
  if (e.ctrlKey) {
    if (ret.length > 0) ret += '+';
    ret += 'ctrl';
    _XTools_js__WEBPACK_IMPORTED_MODULE_3__.XTools.KEY_CTRL = true;
  }
  if (e.shiftKey) {
    if (ret.length > 0) ret += '+';
    ret += 'shift';
    _XTools_js__WEBPACK_IMPORTED_MODULE_3__.XTools.KEY_SHIFT = true;
  }
  if (e.altKey) {
    if (ret.length > 0) ret += '+';
    ret += 'alt';
    _XTools_js__WEBPACK_IMPORTED_MODULE_3__.XTools.KEY_ALT = true;
  }
  if (ret.length > 0) ret += '+';
  ret += e.key;
  GLFrame.pinstance_.callbackKey(1, ret);
  if (!_ir_CanvasInput_js__WEBPACK_IMPORTED_MODULE_0__.CanvasInput.FOCUS) {
  }
  if (ret == 'ctrl+z' || ret == 'ctrl+f' || ret == 'Enter') {
    e.preventDefault();
  }
}

function mainLoop() {
  GLFrame.pinstance_.callbackDraw();
  window.requestAnimationFrame(mainLoop);
}

class GLFrame {
  static gi() {
    return GLFrame.pinstance_;
  }
  constructor() { }

  go(cvs, _draw = null, _touch = null, _key = null, _logic = null) {
    gl = cvs.getContext('webgl', { premultipliedAlpha: false });

    this.pCallbackDraw = _draw;
    this.pCallbackTouch = _touch;
    this.pCallbackKey = _key;
    this.pCallbackLogic = _logic;
    this.pCallbackDropfile = null;

    cvs.addEventListener('touchstart', touchStart);
    cvs.addEventListener('touchmove', touchMove);
    cvs.addEventListener('touchend', touchEnd);

    cvs.addEventListener('mousedown', mouseDown);
    cvs.addEventListener('mousemove', mouseMove);
    cvs.addEventListener('mouseup', mouseUp);
    cvs.addEventListener("mousewheel", mouseWheel);

    cvs.addEventListener("drop", (e) => {
      e.preventDefault();
      GLFrame.gi().callbackDropfile(e.dataTransfer.files, e.offsetX, e.offsetY);
    });
    cvs.addEventListener("dragover", (e) => {
      e.preventDefault();
    });
    cvs.addEventListener("dragenter", (e) => {
      e.preventDefault();
    });
    cvs.focus();

    document.addEventListener('contextmenu', function (e) {
      e.preventDefault();
    });


    window.addEventListener('keydown', keyDown);
    window.addEventListener('keyup', keyUp);
    window.requestAnimationFrame(mainLoop);
  }
  callbackKey(type, code) {
    if (this.pCallbackKey != null) {
      this.pCallbackKey(type, code);
    }
  }
  callbackDraw() {
    gl.clearColor(1.0, 1.0, 1.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    if (this.pCallbackDraw != null) {
      this.pCallbackDraw();
    }
  }

  callbackProctouch(msg, x, y) {
    _XTools_js__WEBPACK_IMPORTED_MODULE_3__.XTools.MOUSE_POS.x = x;
    _XTools_js__WEBPACK_IMPORTED_MODULE_3__.XTools.MOUSE_POS.y = y;
    if (this.pCallbackTouch != null) {
      x = (x * _XDefine_js__WEBPACK_IMPORTED_MODULE_2__.Scr.logicw) / _XDefine_js__WEBPACK_IMPORTED_MODULE_2__.Scr.width;
      y = (y * _XDefine_js__WEBPACK_IMPORTED_MODULE_2__.Scr.logich) / _XDefine_js__WEBPACK_IMPORTED_MODULE_2__.Scr.height;
      this.pCallbackTouch(msg, x, y);
    }
  }
  callbackDropfile(files, x, y) {
    if (this.pCallbackDropfile != null) {
      this.pCallbackDropfile(files, x, y);
    }
  }
  resize() {
    gl.viewport(0, 0, _XDefine_js__WEBPACK_IMPORTED_MODULE_2__.Scr.logicw, _XDefine_js__WEBPACK_IMPORTED_MODULE_2__.Scr.logich);
    _graphics_X2DFast_js__WEBPACK_IMPORTED_MODULE_1__.X2DFast.gi().resetMat();
  }
}

GLFrame.pinstance_ = new GLFrame();


/***/ }),

/***/ "./src/engine/XDefine.js":
/*!*******************************!*\
  !*** ./src/engine/XDefine.js ***!
  \*******************************/
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   "Scr": () => (/* binding */ Scr)
/* harmony export */ });
/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

const { NapiLog } = __webpack_require__(/*! ../ir/NapiLog */ "./src/ir/NapiLog.js");
class Scr {
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


/***/ }),

/***/ "./src/engine/XTools.js":
/*!******************************!*\
  !*** ./src/engine/XTools.js ***!
  \******************************/
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   "XTools": () => (/* binding */ XTools),
/* harmony export */   "fAngle": () => (/* binding */ fAngle),
/* harmony export */   "iDistance": () => (/* binding */ iDistance),
/* harmony export */   "timeMs": () => (/* binding */ timeMs),
/* harmony export */   "freshTime": () => (/* binding */ freshTime),
/* harmony export */   "TimeMS": () => (/* binding */ TimeMS),
/* harmony export */   "RandInt": () => (/* binding */ RandInt),
/* harmony export */   "GetURL": () => (/* binding */ GetURL)
/* harmony export */ });
/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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
class XTools {
  constructor() { }
  static MOUSE_POS = {
    x: 0,
    y: 0,
  };
  static InRect(xx, yy, x, y, w, h) {
    if (xx < x) return false;
    if (yy < y) return false;
    if (xx > x + w) return false;
    if (yy > y + h) return false;
    return true;
  }
  static PROC_TO = 0;
  static CONFIG = null;
  static LoadConfig() {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', "config.json");
    xhr.onreadystatechange = () => {
      if (xhr.readyState === XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          try {
            XTools.CONFIG = JSON.parse(xhr.responseText);
            for (let k in XTools.CONFIG.NodeColor) {
              XTools.CONFIG.NodeColor[k]=parseInt(XTools.CONFIG.NodeColor[k],16);
            }
            for (let k in XTools.CONFIG.LineColor) {
              XTools.CONFIG.LineColor[k]=parseInt(XTools.CONFIG.LineColor[k],16);
            }
          } catch (e) {
            alert('Config file error');
          }
        } else {
          alert('Failed to load config file');
        }
      }
    };
    xhr.send();
  }
}
function fAngle(x, y) {
  return (Math.atan2(-y, x) * 180) / Math.PI;
}
function iDistance(x, y) {
  return Math.sqrt(x * x + y * y);
}

var timeMs = 0;
function freshTime() {
  let t = new Date();
  timeMs = t.getTime();
}
freshTime();
function TimeMS() {
  let t = new Date();
  return t.getTime();
}
function RandInt(min = 0, max = 100) {
  return Math.floor(Math.random() * (max - min)) + min;
}

function GetURL() {
  if ('undefined' != typeof wx) {
    return 'https://7465-testegg-19e3c9-1301193145.tcb.qcloud.la/';
  } else return '';
}


/***/ }),

/***/ "./src/engine/control/XButton.js":
/*!***************************************!*\
  !*** ./src/engine/control/XButton.js ***!
  \***************************************/
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

const { X2DFast } = __webpack_require__(/*! ../graphics/X2DFast */ "./src/engine/graphics/X2DFast.js");

class XButton {
  constructor(x, y, w, h, name) {
    this.pm2f_ = X2DFast.gi();
    this.move(x, y, w, h);
    this.name_ = name;
    this.touchDown_ = false;
    this.clicked_ = false;
    this.rightClicked_ = false;
    this.disable_ = false;
    this.nameColor_ = 0xffffffff;
    this.backgroundColor_ = 0xff487eb8;
  }

  move(x, y, w, h) {
    this.posX_ = x;
    this.posY_ = y;
    this.posW_ = w;
    this.posH_ = h;
    return this;
  }

  draw() {
    let coloroff = 0;
    if (this.touchDown_) coloroff = 0x00202020;
    this.pm2f_.fillRect(
      this.posX_,
      this.posY_,
      this.posW_,
      this.posH_,
      this.backgroundColor_ - coloroff
    );
    if (this.name_ != undefined && this.name_.length > 0)
      this.pm2f_.drawText(
        this.name_,
        14,
        this.posX_ + this.posW_ / 2,
        this.posY_ + this.posH_ / 2 + 2,
        1,
        1,
        0,
        -2,
        -2,
        this.nameColor_ - coloroff
      );
  }

  isTouchIn(x, y) {
    if (x < this.posX_) return false;
    if (y < this.posY_) return false;
    if (x > this.posX_ + this.posW_) return false;
    if (y > this.posY_ + this.posH_) return false;
    return true;
  }
  onTouch(msg, x, y) {
    let isIn = this.isTouchIn(x, y);
    switch (msg) {
      case 1:
        if (isIn) {
          this.touchDown_ = true;
        }
        break;
      case 2:
        break;
      case 3:
        if (this.touchDown_ && isIn) {
          if (this.onClicked_) {
            this.onClicked_();
          }
          else {
            this.clicked_ = true;
          }
        }
        this.touchDown_ = false;
        break;
      case 4:
        if (isIn) {
          this.rightDown_ = true;
        }
        break;
      case 6:
        if (this.rightDown_ && isIn) {
          this.rightClicked_ = true;
        }
        this.rightDown_ = false;
        break;
    }
    return isIn;
  }

  isClicked() {
    if (this.clicked_) {
      this.clicked_ = false;
      return true;
    }
    return false;
  }

  isRightClicked() {
    if (this.rightClicked_) {
      this.rightClicked_ = false;
      return true;
    }
    return false;
  }
}

module.exports = {
  XButton,
};


/***/ }),

/***/ "./src/engine/control/XScroll.js":
/*!***************************************!*\
  !*** ./src/engine/control/XScroll.js ***!
  \***************************************/
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

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

const { X2DFast } = __webpack_require__(/*! ../graphics/X2DFast */ "./src/engine/graphics/X2DFast.js");

class XScroll {
  constructor(options) {
    if (options["type"]) {
      this.type_ = options["type"];
    }
    else {
      this.type_ = "right";
    }
    this.barOff_ = 0;
    this.useScrH_ = false;
  }
  move(x, y, w, h) {
    this.posX_ = x;
    this.posY_ = y;
    this.posW_ = w;
    this.posH_ = h;
    return this;
  }
  draw() {
    X2DFast.gi().fillRect(this.posX_, this.posY_, this.posW_, this.posH_, 0x40808080);
    if (this.type_ == "right") {
      X2DFast.gi().fillRect(this.posX_ + 1, this.posY_ + this.barOff_, this.posW_ - 2, this.posH_ / 3, 0x40000000);
    }
    else if (this.type_ == "button") {
      X2DFast.gi().fillRect(this.posX_ + this.barOff_, this.posY_ + 1, this.posW_ / 3, this.posH_ - 2, 0x40000000);
    }
  }
  isTouchIn(x, y) {
    if (x < this.posX_) return false;
    if (y < this.posY_) return false;
    if (x > this.posX_ + this.posW_) return false;
    if (y > this.posY_ + this.posH_) return false;
    return true;
  }
  setBarOff(rate) {
    if (this.type_ == "right") {
      this.barOff_ = this.posH_ * 2 / 3 * rate;
    }
    else {
      this.barOff_ = this.posW_ * 2 / 3 * rate;
    }
    this.modifyBarOff(0, 0);
  }
  getBarOff() {
    if (this.type_ == "right") {
      return this.barOff_ / (this.posH_ * 2 / 3);
    }
    else {
      return this.barOff_ / (this.posW_ * 2 / 3);
    }
  }
  modifyBarOff(dx, dy) {
    if (this.type_ == "right") {
      this.barOff_ += dy;
      if (this.barOff_ > this.posH_ * 2 / 3) {
        this.barOff_ = this.posH_ * 2 / 3;
      }
    }
    else {
      this.barOff_ += dx;
      if (this.barOff_ > this.posW_ * 2 / 3) {
        this.barOff_ = this.posW_ * 2 / 3;
      }
    }
    if (this.barOff_ < 0) {
      this.barOff_ = 0;
    }
  }
  onTouch(msg, x, y) {
    let isIn = this.isTouchIn(x, y);
    switch (msg) {
      case 10:
        if (this.type_ == "right") {
          this.modifyBarOff(0, -this.posH_ / 3 / 10);
        }
        else if (isIn) {
          this.modifyBarOff(-this.posW_ / 3 / 10, 0);
        }
        break;
      case 11:
        if (this.type_ == "right") {
          this.modifyBarOff(0, this.posH_ / 3 / 10);
        }
        else if (isIn) {
          this.modifyBarOff(this.posW_ / 3 / 10, 0);
        }
        break;
      case 1:
        if (isIn) {
          this.touchDown_ = true;
          if (this.type_ == "right") {
            if (y - this.posY_ < this.barOff_ || y - this.posY_ > this.barOff_ + this.posH_ / 3) {
              this.barOff_ = y - this.posY_ - this.posH_ / 3 / 2;
              this.modifyBarOff(0, 0);
            }
          }
          else {
            if (x - this.posX_ < this.barOff_ || x - this.posX_ > this.barOff_ + this.posW_ / 3) {
              this.barOff_ = x - this.posX_ - this.posW_ / 3 / 2;
              this.modifyBarOff(0, 0);
            }
          }
          this.touchPos_ = {
            x: x,
            y: y,
          }
        }
        break;
      case 2:
        if (this.touchDown_) {
          this.modifyBarOff(x - this.touchPos_.x, y - this.touchPos_.y);
          this.touchPos_.x = x;
          this.touchPos_.y = y;
        }
        break;
      case 3:
        this.touchDown_ = false;
        break;
    }
    return isIn;
  }
}

module.exports = {
  XScroll
}

/***/ }),

/***/ "./src/engine/control/XSelect.js":
/*!***************************************!*\
  !*** ./src/engine/control/XSelect.js ***!
  \***************************************/
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

const { X2DFast } = __webpack_require__(/*! ../graphics/X2DFast */ "./src/engine/graphics/X2DFast.js");

class XSelect {
  constructor(list, default_) {
    this.pm2f_ = X2DFast.gi();
    this.resetList(list, default_);
    this.open_ = false;
    this.nameColor_ = 0xffffffff;
    this.backgroundColor_ = 0xff313131;
    this.tmpSelect_ = -1;
    this.selectCallback = null;
  }
  resetList(list, default_) {
    this.list_ = list;
    this.default_ = default_;
  }
  move(x, y, w, h) {
    this.posX_ = x;
    this.posY_ = y;
    this.posW_ = w;
    this.posH_ = h;
    return this;
  }
  setColor(background, forground) {
    this.nameColor_ = forground;
    this.backgroundColor_ = background;
  }
  draw() {
    let x = this.posX_;
    let y = this.posY_;
    let w = this.posW_;
    let h = this.posH_;
    let model = 3;

    this.pm2f_.fillRect(x, y, w, h, this.backgroundColor_);
    let name = '...';
    if (this.default_.indexOf('\\') != -1) {
      let list = this.default_.split('\\');
      if (list.length > model) {
        for (let i = list.length - model; i < list.length; i++) {
          name += list[i];
          if (i != list.length - 1) {
            name += '\\';
          }
        }
      } else {
        name = this.default_;
      }
    } else {
      name = this.default_;
    }
    this.pm2f_.drawText(name, 16, x, y, 1, 1, 0, -1, -1, this.nameColor_);
    if (this.open_) {
      this.pm2f_.fillRect(
        x,
        y + h,
        w,
        20 * this.list_.length,
        this.backgroundColor_
      );
      for (let i in this.list_) {
        if (i == this.tmpSelect_) {
          this.pm2f_.fillRect(x, y + h + i * 20, w, 20, this.backgroundColor_ + 0x303030);
        }
        
        let name1 = '...';
        if (this.list_[i].indexOf('\\') != -1) {
          let list = this.list_[i].split('\\');
          if (list.length > model) {
            for (let k = list.length - model; k < list.length; k++) {
              name1 += list[k];
              if (k != list.length - 1) {
                name1 += '\\';
              }
            }
          } else {
            name1 = this.list_[i];
          }
        } else {
          name1 = this.list_[i];
        }
        this.pm2f_.drawText(
          name1,
          16,
          x,
          y + h + i * 20,
          1,
          1,
          0,
          -1,
          -1,
          this.nameColor_
        );
      }
    }
  }
  isTouchIn(x, y) {
    if (x < this.posX_) return false;
    if (y < this.posY_) return false;
    if (x > this.posX_ + this.posW_) return false;
    if (y > this.posY_ + this.posH_ + (this.open_ ? 20 * this.list_.length : 0))
      return false;
    return true;
  }
  onTouch(msg, x, y) {
    let isIn = this.isTouchIn(x, y);
    switch (msg) {
      case 1:
        if (!isIn) break;
        if (!this.open_) {
          this.open_ = true;
          break;
        }
        if (this.tmpSelect_ >= 0 && this.tmpSelect_ <= this.list_.length) {
          if (this.default_ != this.list_[this.tmpSelect_]) {
            this.default_ = this.list_[this.tmpSelect_];
            if (this.selectCallback != null) {
              this.selectCallback(this.default_);
            }
          }
        }
        this.open_ = false;
        break;
      case 2:
        if (isIn && this.open_) {
          this.tmpSelect_ = parseInt((y - this.posY_ - this.posH_) / 20);
        }
        break;
      case 3:
        break;
    }
    return isIn;
  }
  registCallback(func) {
    this.selectCallback = func;
  }
}

module.exports = {
  XSelect,
};


/***/ }),

/***/ "./src/engine/graphics/X2DFast.js":
/*!****************************************!*\
  !*** ./src/engine/graphics/X2DFast.js ***!
  \****************************************/
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   "X2DFast": () => (/* binding */ X2DFast)
/* harmony export */ });
/* harmony import */ var _XMat4_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./XMat4.js */ "./src/engine/graphics/XMat4.js");
/* harmony import */ var _XShader_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./XShader.js */ "./src/engine/graphics/XShader.js");
/* harmony import */ var _XDefine_js__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ../XDefine.js */ "./src/engine/XDefine.js");
/* harmony import */ var _XTexture_js__WEBPACK_IMPORTED_MODULE_3__ = __webpack_require__(/*! ./XTexture.js */ "./src/engine/graphics/XTexture.js");
/* harmony import */ var _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__ = __webpack_require__(/*! ../GLFrame.js */ "./src/engine/GLFrame.js");
/* harmony import */ var _XTools_js__WEBPACK_IMPORTED_MODULE_5__ = __webpack_require__(/*! ../XTools.js */ "./src/engine/XTools.js");
/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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








class X2DFast {
  static gi() {
    if (X2DFast.px2f == null) X2DFast.px2f = new X2DFast();
    return X2DFast.px2f;
  }

  constructor() {
    this.localBuffer = _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.createBuffer();
    this.texSampleIdx = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];

    this.vertexArray = new ArrayBuffer(1024 * 1024 * 4 * 2);
    this.vertexFloat32 = new Float32Array(this.vertexArray);
    this.vertexUint32 = new Uint32Array(this.vertexArray);
    this.whiteImg = _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi().loadTextureFromImage('CUSTOM_TEXTURE_1');
    this.whiteCut = _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi().makeCut(this.whiteImg, 0, 0, 1, 1);
    _XShader_js__WEBPACK_IMPORTED_MODULE_1__.XShader.gi();

    this.resetMat();
  }
  resetMat() {
    X2DFast.transform2D.unit();
    X2DFast.transform2D.orthoMat(0, 0, _XDefine_js__WEBPACK_IMPORTED_MODULE_2__.Scr.logicw, _XDefine_js__WEBPACK_IMPORTED_MODULE_2__.Scr.logich);
    let tm = X2DFast.transform2D.mat;
    this.t2dExt = [
      tm[0][0],
      tm[1][0],
      tm[2][0],
      tm[3][0],
      tm[0][1],
      tm[1][1],
      tm[2][1],
      tm[3][1],
      tm[0][2],
      tm[1][2],
      tm[2][2],
      tm[3][2],
      tm[0][3],
      tm[1][3],
      tm[2][3],
      tm[3][3],
    ];
  }

  swapMode2D() {
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.disable(_GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.DEPTH_TEST);

    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.enable(_GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.BLEND);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.blendFunc(_GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.SRC_ALPHA, _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.ONE_MINUS_SRC_ALPHA);// ONE_MINUS_SRC_ALPHA
    // GL_FUNC_ADD、GL_FUNC_SUBTRACT、GL_FUNC_REVERSE_SUBTRACT、GL_MIN、GL_MAX和GL_LOGIC_OP
  }

  DrawCircle(ox, oy, rw, rh, c = 0xffffffff, lw = 1) {
    let lx = -1;
    let ly = -1;
    let i = 0;
    let gap = (Math.PI * 2) / 32;
    while (i < Math.PI * 2 + 0.00001) {
      let dx = Math.cos(i) * rw + ox;
      let dy = Math.sin(i) * rh + oy;
      if (lx != -1) {
        this.drawLine(lx, ly, dx, dy, c, lw);
      }
      lx = dx;
      ly = dy;
      i += gap;
    }
  }

  fillRect(x, y, w, h, c = 0xffffffff) {
    this.drawCut(this.whiteCut, x, y, w, h, 0, 0, 0, c);
  }

  drawLine(x1, y1, x2, y2, c = 0xffffffff, linewidth = 1) {
    x1 = parseInt(x1);
    y1 = parseInt(y1);
    x2 = parseInt(x2);
    y2 = parseInt(y2);
    this.drawCut(
      this.whiteCut,
      x1,
      y1,
      (0,_XTools_js__WEBPACK_IMPORTED_MODULE_5__.iDistance)(x1 - x2, y1 - y2),
      linewidth,
      (0,_XTools_js__WEBPACK_IMPORTED_MODULE_5__.fAngle)(x2 - x1, y2 - y1),
      0,
      0,// +0.5
      c
    );
  }

  drawRect(x, y, w, h, c = 0xffffffff, lw = 1) {
    this.drawLine(x - lw / 2, y, x + w + lw / 2, y, c, lw);
    this.drawLine(x, y, x, y + h, c, lw);
    this.drawLine(x + w, y + h, x + w, y, c, lw);
    this.drawLine(x + w + lw / 2, y + h, x - lw / 2, y + h, c, lw);
  }

  static testTransform(x, y, sw, sh, ra, ox, oy, realw, realh) {
    X2DFast.tmpMat.unit();
    if (ox == -1) ox = 0;
    if (ox == -2) ox = Math.floor(realw / 2);
    if (ox == -3) ox = realw;
    if (oy == -1) oy = 0;
    if (oy == -2) oy = Math.floor(realh / 2);
    if (oy == -3) oy = realh;
    if (ox != 0 || oy != 0) X2DFast.tmpMat.move(-ox, -oy, 0);
    if (sw != 1 || sh != 1) X2DFast.tmpMat.scale(sw, sh, 1);
    if (ra != 0) X2DFast.tmpMat.rotate(0, 0, ra);
    if (x != 0 || y != 0) X2DFast.tmpMat.move(x, y, 0);
  }
  clearBuffer() {
    this.ridDict = {};
    this.ridPoint = 0;
    this.drawCount = 0;
  }
  swapC(c) {
    let r, g, b, a;
    if (isNaN(c)) {
      r = Math.floor(c[0] * 63 / 255);
      g = Math.floor(c[1] * 63 / 255);
      b = Math.floor(c[2] * 63 / 255);
      a = Math.floor(c[3] * 63 / 255);
    }
    else {
      if (c == -1) c = 0xffffffff;
      r = Math.floor((((c >> 16) & 0xff) * 63) / 255);
      g = Math.floor((((c >> 8) & 0xff) * 63) / 255);
      b = Math.floor(((c & 0xff) * 63) / 255);
      a = Math.floor((((c >> 24) & 0xff) * 63) / 255);
    }
    return ((a * 64 + r) * 64 + g) * 64 + b + 0.1;
  }
  drawCut_(pcut, m00, m01, m10, m11, m22, m30, m31, c = 0xffffffff) {
    c = this.swapC(c);
    this.vertexFloat32.set([0.0, 0.0, 0.0, pcut.u0, pcut.v0, m00, m01, m10, m11, m22, m30, m31,
      this.ridDict[pcut.rid], c,
      pcut.w, 0.0, 0.0, pcut.u1, pcut.v1, m00, m01, m10, m11, m22, m30, m31, this.ridDict[pcut.rid], c,
      pcut.w, pcut.h, 0.0, pcut.u2, pcut.v2, m00, m01, m10, m11, m22, m30, m31, this.ridDict[pcut.rid], c,
      0.0, 0.0, 0.0, pcut.u0, pcut.v0, m00, m01, m10, m11, m22, m30, m31, this.ridDict[pcut.rid], c,
      pcut.w, pcut.h, 0.0, pcut.u2, pcut.v2, m00, m01, m10, m11, m22, m30, m31, this.ridDict[pcut.rid], c,
      0.0, pcut.h, 0.0, pcut.u3, pcut.v3, m00, m01, m10, m11, m22, m30, m31, this.ridDict[pcut.rid], c,],
      this.drawCount * 14 * 6);
    this.drawCount += 1;
  }
  drawCutEx(cid, tmat, c = 0xffffffff) {
    let pcut = _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.pinstance_.allCuts[cid];
    if (!(pcut.rid in this.ridDict)) {
      this.ridDict[pcut.rid] = this.ridPoint;
      this.ridPoint += 1;
    }
    tmat = tmat.mat;
    this.drawCut(pcut, tmat[0][0], tmat[0][1], tmat[1][0], tmat[1][1], tmat[2][2], tmat[3][0], tmat[3][1], c);
  }
  drawCut(cid, x = 0, y = 0, sw = 1, sh = 1, ra = 0, ox = 0, oy = 0, c = 0xffffffff) {
    let intX = parseInt(x);
    let intY = parseInt(y);
    let pcut = _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi().allCuts[cid];
    if (pcut == null) return;
    if (!(pcut.rid in this.ridDict)) {
      if (this.ridPoint >= 16) {
        this.freshBuffer();
        this.clearBuffer();
      }
      this.ridDict[pcut.rid] = this.ridPoint;
      this.ridPoint += 1;
    }
    X2DFast.testTransform(intX, intY, sw, sh, ra, ox, oy, pcut.w, pcut.h);
    let tmat = X2DFast.tmpMat.mat;
    this.drawCut_(pcut, tmat[0][0], tmat[0][1], tmat[1][0], tmat[1][1], tmat[2][2], tmat[3][0], tmat[3][1], c);
  }
  drawText(s, size = 14, x = 0, y = 0, sw = 1, sh = 1, ra = 0, ox = 0, oy = 0, c = 0xffffffff) {
    if (s.length <= 0) return 0;
    let cid = _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi().getText(s, size);
    if (cid >= 0) this.drawCut(cid, x, y, sw, sh, ra, ox, oy, c);
    return _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi().allCuts[cid].w;
  }
  getTextWidth(s, size) {
    if (s.length <= 0) return 0;
    let cid = _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi().getText(s, size);
    return _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi().allCuts[cid].w;
  }
  freshBuffer() {
    _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi()._FreshText();
    if (this.drawCount == 0) return;
    let ps = _XShader_js__WEBPACK_IMPORTED_MODULE_1__.XShader.gi().use(_XShader_js__WEBPACK_IMPORTED_MODULE_1__.XShader.ID_SHADER_FAST);
    for (let rid in this.ridDict) {
      _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.activeTexture(_GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.TEXTURE0 + this.ridDict[rid]);
      _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.bindTexture(_GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.TEXTURE_2D, _XTexture_js__WEBPACK_IMPORTED_MODULE_3__.XTexture.gi().ximages[rid].tex);

      _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.uniform1i(ps.tex[this.ridDict[rid]], this.ridDict[rid]);
    }

    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.uniformMatrix4fv(ps.uMat, false, this.t2dExt);

    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.bindBuffer(_GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.ARRAY_BUFFER, this.localBuffer);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.bufferData(_GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.ARRAY_BUFFER, this.vertexArray, _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.STATIC_DRAW);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.vertexAttribPointer(ps.position, 3, _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.FLOAT, false, 4 * 14, 0);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.enableVertexAttribArray(ps.position);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.vertexAttribPointer(ps.aTexCoord, 2, _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.FLOAT, false, 4 * 14, 4 * 3);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.enableVertexAttribArray(ps.aTexCoord);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.vertexAttribPointer(ps.ext1, 4, _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.FLOAT, false, 4 * 14, 4 * 5);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.enableVertexAttribArray(ps.ext1);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.vertexAttribPointer(ps.ext2, 4, _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.FLOAT, false, 4 * 14, 4 * 9);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.enableVertexAttribArray(ps.ext2);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.vertexAttribPointer(ps.inColor, 1, _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.FLOAT, false, 4 * 14, 4 * 13);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.enableVertexAttribArray(ps.inColor);

    _GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.drawArrays(_GLFrame_js__WEBPACK_IMPORTED_MODULE_4__.gl.TRIANGLES, 0, 6 * this.drawCount);
  }
}
X2DFast.tmpMat = new _XMat4_js__WEBPACK_IMPORTED_MODULE_0__.XMat4();
X2DFast.transform2D = new _XMat4_js__WEBPACK_IMPORTED_MODULE_0__.XMat4();

X2DFast.px2f = null;


/***/ }),

/***/ "./src/engine/graphics/XMat4.js":
/*!**************************************!*\
  !*** ./src/engine/graphics/XMat4.js ***!
  \**************************************/
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   "XMat4": () => (/* binding */ XMat4)
/* harmony export */ });
/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

class XMat4 {
  constructor() {
    this.unit();
  }

  Get() {
    return this.mat;
  }
  unit() {
    this.mat = [
      [1, 0, 0, 0],
      [0, 1, 0, 0],
      [0, 0, 1, 0],
      [0, 0, 0, 1],
    ];
    return this;
  }
  copy(m) {
    for (let i = 0; i < 4; i++) {
      for (let j = 0; j < 4; j++) {
        this.mat[i][j] = m.mat[i][j];
      }
    }
  }
  initRotateMatX(hd) {
    this.unit();
    let _cos = Math.cos(hd);
    let _sin = Math.sin(hd);
    this.mat[1][1] = _cos;
    this.mat[1][2] = -_sin;
    this.mat[2][1] = _sin;
    this.mat[2][2] = _cos;
  }
  initRotateMatY(hd) {
    this.unit();
    let _cos = Math.cos(hd);
    let _sin = Math.sin(hd);
    this.mat[0][0] = _cos;
    this.mat[0][2] = -_sin;
    this.mat[2][0] = _sin;
    this.mat[2][2] = _cos;
  }

  initRotateMatZ(hd) {
    this.unit();
    let _cos = Math.cos(hd);
    let _sin = Math.sin(hd);
    this.mat[0][0] = _cos;
    this.mat[0][1] = -_sin;
    this.mat[1][0] = _sin;
    this.mat[1][1] = _cos;
  }
  initScaleMat(x, y, z) {
    this.unit();
    this.mat[0][0] = x;
    this.mat[1][1] = y;
    this.mat[2][2] = z;
  }
  move(x, y, z = 0) {
    this.mat[3][0] += x;
    this.mat[3][1] += y;
    this.mat[3][2] += z;
    return this;
  }
  rotate(x, y, z) {
    if (x != 0) {
      tmpmat.initRotateMatX((x * Math.PI) / 180);
      this.mult(tmpmat);
    }
    if (y != 0) {
      tmpmat.initRotateMatY((y * Math.PI) / 180);
      this.mult(tmpmat);
    }
    if (z != 0) {
      tmpmat.initRotateMatZ((z * Math.PI) / 180);
      this.mult(tmpmat);
    }
    return this;
  }

  scale(x, y, z = 1) {
    tmpmat.initScaleMat(x, y, z);
    this.mult(tmpmat);
    return this;
  }

  mult(m4) {
    let tmp = [
      [1, 0, 0, 0],
      [0, 1, 0, 0],
      [0, 0, 1, 0],
      [0, 0, 0, 1],
    ];
    for (let i = 0; i < 4; i++) {
      for (let j = 0; j < 4; j++) {
        tmp[i][j] =
          this.mat[i][0] * m4.mat[0][j] +
          this.mat[i][1] * m4.mat[1][j] +
          this.mat[i][2] * m4.mat[2][j] +
          this.mat[i][3] * m4.mat[3][j];
      }
    }
    this.mat = tmp;
    return this;
  }

  MultRight(m4) {
    this.mat = np.dot(m4.mat, this.mat);
    return this;
  }

  PerspectiveMatrix(n, f, w = -1, h = -1) {
    if (w == -1) w = Scr.logicw;
    if (h == -1) h = Scr.logich;
    let ret = w / (tan((30 * pi) / 180) * 2);
    this.unit();
    this.mat[0][0] = 2 / w;
    this.mat[1][1] = 2 / h;

    this.mat[2][2] = 1 / (f - n);
    this.mat[2][3] = 1 / ret; //#2 / f
    this.mat[3][2] = -n / (f - n);
    this.mat[3][3] = 1;
    return ret;
  }

  orthoMat(x, y, w, h) {
    this.move(-w / 2 - x, -h / 2 - y, 0);
    this.scale(2 / w, -2 / h, 0);
  }

  Make2DTransformMat(
    mx = 0,
    my = 0,
    sw = 1,
    sh = 1,
    ra = 0,
    ox = 0,
    oy = 0,
    realw = 0,
    realh = 0
  ) {
    this.unit();
    if (ox == -1) ox = 0;
    if (ox == -2) ox = realw / 2;
    if (ox == -3) ox = realw;
    if (oy == -1) oy = 0;
    if (oy == -2) oy = realh / 2;
    if (oy == -3) oy = realh;
    if (ox != 0 || oy != 0) this.move(-ox, -oy, 0);
    if (sw != 1 || sh != 1) this.scale(sw, sh, 1);
    if (ra != 0) this.rotate(0, 0, ra);
    if (mx != 0 || my != 0) this.move(mx, my, 0);
  }

  Make2DTransformMat_(mx, my, sw, sh, ra, ox = 0, oy = 0) {
    this.unit();
    if (mx != 0 || my != 0) this.move(-mx, -my, 0);
    if (ra != 0) this.rotate(0, 0, -ra);
    if (sw != 1 || sh != 1) this.scale(1 / sw, 1 / sh, 1);
    return this;
  }
}

var tmpmat = new XMat4();


/***/ }),

/***/ "./src/engine/graphics/XShader.js":
/*!****************************************!*\
  !*** ./src/engine/graphics/XShader.js ***!
  \****************************************/
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   "XShader": () => (/* binding */ XShader)
/* harmony export */ });
/* harmony import */ var _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ../GLFrame.js */ "./src/engine/GLFrame.js");
/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

const { shaderFastVs, shaderFastFs } = __webpack_require__(/*! ./shaders/shader_fast */ "./src/engine/graphics/shaders/shader_fast.js");
const { NapiLog } = __webpack_require__(/*! ../../ir/NapiLog.js */ "./src/ir/NapiLog.js");


class XShader {
  static gi() {
    if (XShader.pinstance_ == null) XShader.pinstance_ = new XShader();
    return XShader.pinstance_;
  }
  constructor() {
    this.pUseingShader = null;
    this.shaders = [];
    this.initFastShader();
  }
  initFastShader() {
    this.shaderFast = { program: this.initShader(shaderFastVs, shaderFastFs) };

    this.shaderFast.position = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getAttribLocation(
      this.shaderFast['program'],
      'position'
    );
    this.shaderFast.aTexCoord = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getAttribLocation(
      this.shaderFast['program'],
      'aTexCoord'
    );
    this.shaderFast.ext1 = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getAttribLocation(
      this.shaderFast['program'],
      'ext1'
    );
    this.shaderFast.ext2 = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getAttribLocation(
      this.shaderFast['program'],
      'ext2'
    );
    this.shaderFast.inColor = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getAttribLocation(
      this.shaderFast['program'],
      'inColor'
    );

    this.shaderFast.uMat = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getUniformLocation(
      this.shaderFast['program'],
      'uMat'
    );

    this.shaderFast.tex = {};
    for (let i = 0; i < 16; i++) {
      this.shaderFast.tex[i] = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getUniformLocation(
        this.shaderFast['program'],
        'tex' + i
      );
    }

    this.shaders[XShader.ID_SHADER_FAST] = this.shaderFast;
    this.use(XShader.ID_SHADER_FAST);
  }
  use(sid) {
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.useProgram(this.shaders[sid]['program']);
    this.pUseingShader = this.shaders[sid];
    return this.pUseingShader;
  }
  initShader(vss, fss) {
    var vs = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.createShader(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.VERTEX_SHADER);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.shaderSource(vs, vss);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.compileShader(vs);
    if (!_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getShaderParameter(vs, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.COMPILE_STATUS)) {
      NapiLog.logError(
        'error occured compiling the shaders:' + _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getShaderInfoLog(vs)
      );
      return null;
    }

    var fs = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.createShader(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.FRAGMENT_SHADER);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.shaderSource(fs, fss);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.compileShader(fs);
    if (!_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getShaderParameter(fs, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.COMPILE_STATUS)) {
      NapiLog.logError(
        'error occured compiling the shaders:' + _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getShaderInfoLog(fs)
      );
      return null;
    }

    var ret = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.createProgram();

    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.attachShader(ret, vs);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.attachShader(ret, fs);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.linkProgram(ret);

    if (!_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.getProgramParameter(ret, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.LINK_STATUS)) {
      NapiLog.logError('unable to initialize!');
      return;
    }

    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.deleteShader(vs);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.deleteShader(fs);
    return ret;
  }
}

XShader.ID_SHADER_FAST = 2;

XShader.pinstance_ = null;


/***/ }),

/***/ "./src/engine/graphics/XTexture.js":
/*!*****************************************!*\
  !*** ./src/engine/graphics/XTexture.js ***!
  \*****************************************/
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export */ __webpack_require__.d(__webpack_exports__, {
/* harmony export */   "XTexture": () => (/* binding */ XTexture)
/* harmony export */ });
/* harmony import */ var _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ../GLFrame.js */ "./src/engine/GLFrame.js");
/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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



class XTexture {
  static gi() {
    if (XTexture.pinstance_ == null) XTexture.pinstance_ = new XTexture();
    return XTexture.pinstance_;
  }
  constructor() {
    this.ximages = [];
    this.allCuts = {};
    this.tmpCutid = 0;
    this.aiCutid = 100;

    this.textImgs = {};
    this.textIdxs = {};

    this.textTmpRid = this.loadTexture(1024, 256);
    this.bfirst = true;

    this.textCvs = document.createElement('canvas');
    this.textCvs.width = 1024;
    this.textCvs.height = 256;
    this.textCtx = this.textCvs.getContext('2d', { willReadFrequently: true });
    this.textCtx.textBaseline = 'top';
    this.textCtx.textAlign = 'left';
  }
  static initTextureStatus(tex) {
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.activeTexture(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE0);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.bindTexture(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D, tex);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.pixelStorei(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.texParameteri(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_MIN_FILTER, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.NEAREST);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.texParameteri(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_MAG_FILTER, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.NEAREST);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.texParameteri(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_WRAP_S, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.CLAMP_TO_EDGE);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.texParameteri(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_WRAP_T, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.CLAMP_TO_EDGE);
  }
  loadTextureFromImage(path, keepdata = false) {
    if (path == 'CUSTOM_TEXTURE_1') {
      var rid = this.ximages.length;

      var texture = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.createTexture();
      XTexture.initTextureStatus(texture);

      let tmp = new Uint8Array([255, 255, 255, 255]);
      _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.texImage2D(
        _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D,
        0,
        _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.RGBA,
        1,
        1,
        0,
        _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.RGBA,
        _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.UNSIGNED_BYTE,
        tmp
      );

      this.ximages[rid] = { stat: 1, path: path, tex: texture, w: 1, h: 1 };
      return rid;
    } else {
      for (let i = 0; i < this.ximages.length; i++) {
        if (this.ximages[i]['path'] == path) {
          return i;
        }
      }
      var rid = this.ximages.length;
      this.ximages[rid] = { stat: 0, path: path, tex: null };
      var image = new Image();
      image.src = path; //"http://localhost:8910/"+
      image.onload = function () {
        var texture = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.createTexture();
        XTexture.initTextureStatus(texture);

        _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.texImage2D(
          _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D,
          0,
          _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.RGBA,
          _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.RGBA,
          _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.UNSIGNED_BYTE,
          image
        );

        XTexture.pinstance_.ximages[rid].tex = texture;
        XTexture.pinstance_.ximages[rid].img = image;
        XTexture.pinstance_.ximages[rid].stat = 1;
        XTexture.pinstance_.ximages[rid].w = image.width;
        XTexture.pinstance_.ximages[rid].h = image.height;
      };
      return rid;
    }
  }
  TmpCut(rid, x = 0, y = 0, w = -1, h = -1, ww = 1024, hh = 1024) {
    if (this.ximages[rid].stat != 1) return -1;

    if (w == -1) w = ww;
    if (h == -1) h = hh;
    this.allCuts[this.tmpCutid] = {
      rid: rid,
      x: x,
      y: y,
      w: w,
      h: h,
      u0: x / ww,
      v0: y / hh,
      u1: (x + w) / ww,
      v1: y / hh,
      u2: (x + w) / ww,
      v2: (y + h) / hh,
      u3: x / ww,
      v3: (y + h) / hh,
    };
    this.tmpCutid += 1;
    return this.tmpCutid - 1;
  }
  makeCut(rid, x = 0, y = 0, w = -1, h = -1, ww = -1, hh = -1) {
    if (ww == -1) ww = this.ximages[rid].w;
    if (hh == -1) hh = this.ximages[rid].h;
    if (w == -1) w = ww;
    if (h == -1) h = hh;
    this.allCuts[this.aiCutid] = {
      rid: rid,
      x: x,
      y: y,
      w: w,
      h: h,
      u0: x / ww,
      v0: y / hh,
      u1: (x + w) / ww,
      v1: y / hh,
      u2: (x + w) / ww,
      v2: (y + h) / hh,
      u3: x / ww,
      v3: (y + h) / hh,
    };
    this.aiCutid += 1;
    return this.aiCutid - 1;
  }
  timenow() {
    let myDate = new Date();
    return myDate.getTime() / 1000;
  }

  PutTexture(tex, w, h) {
    var rid = this.ximages.length;
    this.ximages[rid] = { stat: 1, path: 'put' + rid, tex: tex, w: w, h: h };
    return rid;
  }

  loadTexture(width, height) {
    var rid = this.ximages.length;

    var texture = _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.createTexture();
    XTexture.initTextureStatus(texture);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.texImage2D(
      _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D,
      0,
      _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.RGBA,
      width,
      height,
      0,
      _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.RGBA,
      _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.UNSIGNED_BYTE,
      null
    );

    this.ximages[rid] = {
      stat: 1,
      path: 'default' + rid,
      tex: texture,
      w: width,
      h: height,
    };
    return rid;
  }
  initTextImageData(s, size) {
    this.textCtx.clearRect(0, 0, 1024, 256);
    this.textCtx.font = size + "px '宋体'";
    this.textCtx.fillStyle = 'rgba(255,255,255,1)';
    this.textCtx.fillText(s, 1, 1);
    let imgd = this.textCtx.getImageData(0, 0, 1024, 256).data;
    let w = 1024;
    let h = size + 5;
    let x = 256;
    while (x == 256) {
      h -= 1;
      for (x = 0; x < 128; x++) {
        let p = (h * 1024 + x) * 4;
        if (imgd[p] != 0) break;
      }
    }
    let y = h;
    while (y == h) {
      w -= 1;
      for (y = 0; y < h; y++) {
        let p = (y * 1024 + w) * 4;
        if (imgd[p] != 0) break;
      }
    }
    return this.textCtx.getImageData(0, 0, w + 1, h + 1);
  }
  getText(s, size) {
    let textIdx = s + size;

    if (textIdx in this.textIdxs) {
      this.textIdxs[textIdx].time = this.timenow();
      return this.textIdxs[textIdx].cid;
    }
    let imgd = this.initTextImageData(s, size);
    let w = imgd.width;
    let h = imgd.height;
    let useHeight = Math.floor((h + 31) / 32);
    let mask = 0;
    for (let i = 0; i < useHeight; i++) mask |= 1 << i;
    let rid = -1;
    let off = -1;
    for (let k in this.textImgs) {
      for (let i = 0; i < 32 - useHeight + 1; i++) {
        if ((this.textImgs[k].mask & (mask << i)) == 0) {
          off = i;
          break;
        }
      }
      if (off != -1) {
        rid = k;
        break;
      }
    }
    if (rid == -1) {
      rid = this.loadTexture(1024, 1024);
      this.textImgs[rid] = { mask: 0 };
      off = 0;
    }
    let cid = this.makeCut(rid, 0, off * 32, w, h);
    this.textImgs[rid]['mask'] |= mask << off;
    this.textIdxs[textIdx] = { cid: cid, rid: rid, mask: mask << off, time: this.timenow(), };
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.activeTexture(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE0);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.bindTexture(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D, this.ximages[rid].tex);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.pixelStorei(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
    _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.texSubImage2D(_GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.TEXTURE_2D, 0, 0, off * 32, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.RGBA, _GLFrame_js__WEBPACK_IMPORTED_MODULE_0__.gl.UNSIGNED_BYTE, imgd);
    return cid;
  }
  _FreshText() {
    this.tmpCutid = 0;
    let nt = this.timenow();
    let rm = [];
    for (let idx in this.textIdxs) {
      if (nt - this.textIdxs[idx].time > 3) {
        this.textImgs[this.textIdxs[idx].rid].mask &= ~this.textIdxs[idx].mask;
        delete this.allCuts[this.textIdxs[idx].cid];
        rm.push(idx);
      }
    }
    for (let idx in rm) {
      delete this.textIdxs[rm[idx]];
    }
  }
  static ExpandColor(c) {
    return [
      ((c >> 16) & 0xff) / 255,
      ((c >> 8) & 0xff) / 255,
      (c & 0xff) / 255,
      ((c >> 24) & 0xff) / 255];//r,g,b,a
  }
}
XTexture.pinstance_ = null;


/***/ }),

/***/ "./src/engine/graphics/shaders/shader_fast.js":
/*!****************************************************!*\
  !*** ./src/engine/graphics/shaders/shader_fast.js ***!
  \****************************************************/
/***/ ((module) => {

/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

var shaderFastVs = `
attribute vec3 position;
attribute vec2 aTexCoord;
attribute vec4 ext1;//x,y,sw,sh,
attribute vec4 ext2;//ra,ox,oy,i
attribute float inColor;
uniform mat4 uMat;

varying vec2 TexCoord;
varying float tex_point;
varying vec4 clr_filter;
void main()
{
    mat4 tmpMat=mat4(ext1[0],ext1[2],0.0,ext2[1],
                      ext1[1],ext1[3],0.0,ext2[2],
                      0.0,0.0,ext2[0],0.0,
                      0.0,0.0,0.0,1.0);
    
    vec4 tv=vec4(position.x, position.y, position.z, 1.0)*tmpMat*uMat;
    gl_Position = tv;
    TexCoord = aTexCoord;
    tex_point = ext2[3];

    clr_filter=vec4(0.0,0.0,0.0,1.0);
    int tt=int(inColor);

    int tt1=tt-tt/64*64;
    clr_filter.b=0.015873015873*float(tt1);
    tt=tt/64;

    int tt2=tt-tt/64*64;
    clr_filter.g=0.015873015873*float(tt2);
    tt=tt/64;

    int tt3=tt-tt/64*64;
    clr_filter.r=0.015873015873*float(tt3);
    tt=tt/64;
    
    clr_filter.a=0.015873015873*float(tt-tt/64*64);
}
`;

var shaderFastFs = `
precision mediump float;

varying vec2 TexCoord;
varying float tex_point;
varying vec4 clr_filter;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;
uniform sampler2D tex8;
uniform sampler2D tex9;
uniform sampler2D tex10;
uniform sampler2D tex11;
uniform sampler2D tex12;
uniform sampler2D tex13;
uniform sampler2D tex14;
uniform sampler2D tex15;

void main()
{
    if(tex_point<0.5)gl_FragColor = texture2D(tex0, TexCoord);
    else if(tex_point<1.5)gl_FragColor = texture2D(tex1, TexCoord);
    else if(tex_point<2.5)gl_FragColor = texture2D(tex2, TexCoord);
    else if(tex_point<3.5)gl_FragColor = texture2D(tex3, TexCoord);
    else if(tex_point<4.5)gl_FragColor = texture2D(tex4, TexCoord);
    else if(tex_point<5.5)gl_FragColor = texture2D(tex5, TexCoord);
    else if(tex_point<6.5)gl_FragColor = texture2D(tex6, TexCoord);
    else if(tex_point<7.5)gl_FragColor = texture2D(tex7, TexCoord);
    else if(tex_point<8.5)gl_FragColor = texture2D(tex8, TexCoord);
    else if(tex_point<9.5)gl_FragColor = texture2D(tex9, TexCoord);
    else if(tex_point<10.5)gl_FragColor = texture2D(tex10, TexCoord);
    else if(tex_point<11.5)gl_FragColor = texture2D(tex11, TexCoord);
    else if(tex_point<12.5)gl_FragColor = texture2D(tex12, TexCoord);
    else if(tex_point<13.5)gl_FragColor = texture2D(tex13, TexCoord);
    else if(tex_point<14.5)gl_FragColor = texture2D(tex14, TexCoord);
    else if(tex_point<15.5)gl_FragColor = texture2D(tex15, TexCoord);
    gl_FragColor=gl_FragColor * clr_filter;
}`;

module.exports = {
  shaderFastVs,
  shaderFastFs,
};


/***/ }),

/***/ "./src/ir/CanvasInput.js":
/*!*******************************!*\
  !*** ./src/ir/CanvasInput.js ***!
  \*******************************/
/***/ ((module) => {

/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

class CanvasInput {
  static FOCUS = false;
  static SAFE_AREA = [];
  static SetSafeArea(x, y, w, h) {
    CanvasInput.SAFE_AREA = [x, y, w, h];
  }
  static Reset(x, y, w, h, value, cb, cb2, cb3) {
    CanvasInput.SAFE_AREA = [x, y, w, h];
    let ci = document.getElementById('canvas_textarea');
    ci.style.left = x + 'px';
    ci.style.top = y + 'px';
    ci.style.width = w + 'px';
    ci.style.height = h + 'px';
    ci.style.display = 'block';
    ci.value = value;
    ci.onchange = (e) => {
      if (cb) {
        cb(e.target.value);
      }
    };
    ci.oninput = (e) => {
      if (cb2) {
        cb2(e.target.value);
      }
    };
    ci.focus();

    ci.addEventListener('keydown', (e) => {
      if (e.key == 'Enter') {
        console.log(e);
        e.preventDefault();
        if (cb3) {
          cb3();
        }
      }
    });
    CanvasInput.FOCUS = true;
  }
  static HideEx() {
    let ci = document.getElementById('canvas_textarea');
    ci.style.display = 'none';
    CanvasInput.FOCUS = false;
  }
}

module.exports = {
  CanvasInput,
};


/***/ }),

/***/ "./src/ir/IrToPicture.js":
/*!*******************************!*\
  !*** ./src/ir/IrToPicture.js ***!
  \*******************************/
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

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

const { X2DFast } = __webpack_require__(/*! ../engine/graphics/X2DFast */ "./src/engine/graphics/X2DFast.js");

const INTYPE = {
  state: 0,
  depend: 1,
  value: 2,
  framestate: 3,
  root: 4,
  other: 5,
}

class NODE_TYPE_MASK {
  static NONE = 0;
  static CONTROL = 1 << INTYPE.state;
  static DEPEND = 1 << INTYPE.depend;
  static VALUE = 1 << INTYPE.value;
  static FRAMESTATE = 1 << INTYPE.framestate;
  static ROOT = 1 << INTYPE.root;
  static OTHER = 1 << INTYPE.other;
}

class IrToPicture {
  static INVALID_DEEP = -99999;
  static NODEH = 20;
  static LINE_TYPE = ["state", "depend", "value", "framestate", "root"];
  static nodeType(ir) {
    if (ir.op == "STATE_ENTRY") {
      return "control";
    }
    if (ir.in[INTYPE.state].length > 0 &&
      ["VALUE_SELECTOR", "DEPEND_SELECTOR", "DEPEND_RELAY", "LOOP_EXIT_DEPEND", "LOOP_EXIT_VALUE", "STATE_SPLIT", "GET_EXCEPTION"].indexOf(ir.op) == -1) {
      return "control";

    }
    if (ir.op == "DEPEND_ENTRY" || ir.in[INTYPE.depend].length > 0) {
      return "depend";
    }
    if (ir.op == "ARG" || ir.op == "CONSTANT" || ir.op == "CONST_DATA" || ir.in[INTYPE.value].length > 0) {
      return "value";
    }
    return "other";
  }
  static nodeTypeMask(ir) {
    let mask = NODE_TYPE_MASK.NONE;
    if (ir.op == "STATE_ENTRY") {
      mask |= NODE_TYPE_MASK.CONTROL;
    }
    if (ir.in[INTYPE.state].length > 0 &&
      ["VALUE_SELECTOR", "DEPEND_SELECTOR", "DEPEND_RELAY", "LOOP_EXIT_DEPEND", "LOOP_EXIT_VALUE", "STATE_SPLIT", "GET_EXCEPTION"].indexOf(ir.op) == -1) {
      mask |= NODE_TYPE_MASK.CONTROL;
    }
    if (ir.op == "DEPEND_ENTRY" || ir.in[INTYPE.depend].length > 0) {
      mask |= NODE_TYPE_MASK.DEPEND;
    }
    if (ir.op == "ARG" || ir.op == "CONSTANT" || ir.op == "CONST_DATA" || ir.in[INTYPE.value].length > 0) {
      mask |= NODE_TYPE_MASK.VALUE;
    }
    if (ir.op == "FRAME_STATE" || ir.in[INTYPE.framestate].length > 0) {
      mask |= NODE_TYPE_MASK.FRAMESTATE;
    }
    if (ir.op == "CIRCUIT_ROOT" || ir.in[INTYPE.root].length > 0) {
      mask |= NODE_TYPE_MASK.ROOT;
    }
    if (mask == NODE_TYPE_MASK.NONE) {
      mask = NODE_TYPE_MASK.OTHER;
    }
    return mask;
  }
  static isLoopBack(l, nodes) {
    if (nodes[l.toId].ir.op == "LOOP_BEGIN" && l.fromId == nodes[l.toId].ir.in[0][1]) {
      return true;
    }
    if (nodes[l.toId].ir.op == "DEPEND_SELECTOR" && l.fromId == nodes[l.toId].ir.in[1][1]) {
      return true;
    }
    if (nodes[l.toId].ir.op == "VALUE_SELECTOR" && l.fromId == nodes[l.toId].ir.in[2][1]) {
      return true;
    }
    return false;
  }
  static toPicture(irList, type, isBlock) {
    let nodes = {};
    let entry = -1;
    for (let ir of irList) {//用于生成图的所有节点
      if (type == 0) {//仅控制流
        if (this.nodeType(ir) != "control") continue;
      }
      let name = ir.id + "," + ir.op;
      if (ir.op == "JS_BYTECODE") {
        name = ir.id + "," + ir.bytecode;
      }
      nodes[ir.id] = {
        type: this.nodeType(ir),
        mask: this.nodeTypeMask(ir),
        hide: false,
        inCount: 0,
        in: [],
        inh: {},
        outCount: 0,
        out: [],
        outh: [],
        pos: { x: -1, y: -1 },
        deep: this.INVALID_DEEP,
        name: name,
        nameWidth: X2DFast.gi().getTextWidth(name, 14),
        ir: ir,
      }
      if (entry == -1) {
        entry = ir.id;
      }
    }

    let lines = [];
    let lid = 0;
    for (let i in nodes) {//生成连接线
      let inId = parseInt(i);
      for (let inP1 = 0; inP1 < nodes[inId].ir.in.length; inP1++) {
        for (let inP2 = 0; inP2 < nodes[inId].ir.in[inP1].length; inP2++) {
          let outId = nodes[inId].ir.in[inP1][inP2];
          if (outId in nodes) {
            let line = {
              lid: lid++,
              lineType: this.LINE_TYPE[inP1],
              inNum: nodes[inId].inCount,
              outNum: nodes[outId].outCount,
              fromId: outId,
              toId: inId,
              inP1: inP1,
              inP2: inP2,
              outP: nodes[outId].ir.out.indexOf(inId),
              used: false,
            };
            nodes[inId].inCount++;
            nodes[inId].in.push(line);
            nodes[outId].outCount++;
            nodes[outId].out.push(line);
            lines.push(line);
          }
        }
      }
    }

    this.resetPicture(nodes, isBlock);

    return {
      nodes: nodes,
      lines: lines,
    };
  }
  static deepTest(n, nodes, isBlock, stack, dist) {
    try {
      stack.push(n.ir.id);
    }
    catch (e) {
      console.log(1);
    }
    if (stack.length > Object.keys(nodes).length * 2) {
      return true;
    }
    if (stack.length > 1 && n.ir.id == dist) {
      return true;
    }
    for (let i = 0; i < n.out.length; i++) {
      let nout = nodes[n.out[i].toId];
      if (n.deep != this.INVALID_DEEP) {
        if (nout.deep == this.INVALID_DEEP) {
          nout.deep = n.deep + 1;
          if (this.deepTest(nout, nodes, isBlock, stack, dist)) {
            return true;
          }
        }
        if (nout.deep <= n.deep) {
          if (!this.isLoopBack(n.out[i], nodes) && !isBlock) {
            nout.deep = n.deep + 1;
            if (this.deepTest(nout, nodes, isBlock, stack, dist)) {
              return true;
            }
          }
        }
      }
    }
    stack.pop();
    return false;
  }
  static checkoutLoop(ls) {
    console.log(JSON.stringify(ls));
    let dicts = {};
    for (let l of ls) {
      if (!(l in dicts)) {
        dicts[l] = 1;
      }
      else {
        dicts[l]++;
      }
    }
    console.log(JSON.stringify(dicts, null, 4));
  }
  static TEST_LOOP = true;
  static resetPicture(nodes, isBlock) {
    if (this.TEST_LOOP && Object.keys(nodes).length > 0) {
      for (let k in nodes) {
        if (k == 0) {
          nodes[k].deep = 0;
        }
        else {
          nodes[k].deep = this.INVALID_DEEP;
        }
      }
      let testResult = [];
      this.deepTest(nodes[0], nodes, isBlock, testResult, 0);
      if(testResult.length>0){
        this.checkoutLoop(testResult);
      }
    }

    let entry = true;
    let enums = [];
    for (let k in nodes) {
      let n = nodes[k];
      if (n.hide) {
        continue;
      }
      if (entry) {
        n.pos.x = 0;
        n.pos.y = 0;
        n.deep = 0;
        entry = false;
        enums.push(k);
      }
      else {
        n.deep = this.INVALID_DEEP;
      }
    }
    let collectDebug = [];
    while (enums.length > 0) {//12,18,27,28,31,34
      let nextenums = [];
      for (let e = 0; e < enums.length; e++) {
        let k = enums[e];
        let n = nodes[k];
        if (n.hide) {
          continue;
        }
        for (let i = 0; i < n.out.length; i++) {
          let nout = nodes[n.out[i].toId];
          if (n.deep != this.INVALID_DEEP) {
            if (nout.deep == this.INVALID_DEEP) {
              nout.deep = n.deep + 1;
              nextenums.push(nout.ir.id);
            }
            if (nout.deep <= n.deep) {
              if (!this.isLoopBack(n.out[i], nodes) && !isBlock) {
                nout.deep = n.deep + 1;
                nextenums.push(nout.ir.id);
              }
            }
          }
        }
        for (let i = 0; i < n.in.length; i++) {
          let nin = nodes[n.in[i].fromId];
          if (n.deep != this.INVALID_DEEP) {
            if (nin.deep == this.INVALID_DEEP) {
              nin.deep = n.deep - 1;
              nextenums.push(nin.ir.id);
            }
            if (nin.deep >= n.deep) {
              if (!this.isLoopBack(n.in[i], nodes) && !isBlock) {
                n.deep = nin.deep + 1;
                nextenums.push(n.ir.id);
              }
            }
          }
        }
      }
      collectDebug.push(enums);

      enums = nextenums;
    }

    let levels = {};
    for (let k in nodes) {//节点分层
      let n = nodes[k];
      if (n.hide) {
        continue;
      }
      if (!(n.deep in levels)) {
        levels[n.deep] = [];
      }
      levels[n.deep].push(n);
    }
    let ty = 50;
    for (let k in nodes) {
      let n = nodes[k];
      let ltypes = [];
      for (let l of n.out) {
        if (ltypes.indexOf(l.lineType) < 0) {
          ltypes.push(l.lineType);
        }
      }
      n.ltypes = ltypes;
      if (n.hide) {
        continue;
      }
      if (n.deep == this.INVALID_DEEP) {
        n.pos.x = this.INVALID_DEEP;//Scr.logicw - 100;
        n.pos.y = ty;
        ty += 50;
      }
    }
    let posy = 0;
    let ks = Object.keys(levels).sort((a, b) => { return parseInt(a) - parseInt(b) });
    for (let k of ks) {
      k = parseInt(k);
      if (k == this.INVALID_DEEP) {
        continue;
      }
      let inCount = 0;
      let outCount = 0;
      let inP = 0;
      for (let i = 0; i < levels[k].length; i++) {
        let n = levels[k];
        if (n.hide) {
          continue;
        }
        for (let j = 0; j < n[i].in.length; j++) {
          let l = n[i].in[j];
          if (!n[i].inh[l.fromId + l.lineType]) {
            n[i].inh[l.fromId + l.lineType] = (inP + 1) * 5;
            inP += 1;
          }
        }
        inCount += Object.keys(n[i].inh).length;

        outCount += n[i].ltypes.length;
      }
      posy += (inCount + 1) * 5;

      let outP = 0;
      for (let i = 0; i < levels[k].length; i++) {
        let n = levels[k];
        if (n.hide) {
          continue;
        }
        for (let j = 0; j < n[i].out.length; j++) {
          n[i].outh[j] = (outP + 1 + n[i].ltypes.indexOf(n[i].out[j].lineType)) * 5;
        }
        n[i].pos.y = posy;
        outP += n[i].ltypes.length;
      }

      posy += (outCount + 1) * 5 + this.NODEH;

      let w = 0;
      for (let i = 0; i < levels[k].length; i++) {//当前行总宽度
        w += levels[k][i].nameWidth + 20;
      }
      let x = -w / 2;
      for (let i = 0; i < levels[k].length; i++) {//每个节点x偏移
        levels[k][i].pos.x = x + 10;
        x += levels[k][i].nameWidth + 20;
      }
    }
  }
}

module.exports = {
  IrToPicture
}

/***/ }),

/***/ "./src/ir/IrViewer.js":
/*!****************************!*\
  !*** ./src/ir/IrViewer.js ***!
  \****************************/
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

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

const { Scr } = __webpack_require__(/*! ../engine/XDefine */ "./src/engine/XDefine.js");
const { XTools } = __webpack_require__(/*! ../engine/XTools */ "./src/engine/XTools.js");
const { XButton } = __webpack_require__(/*! ../engine/control/XButton */ "./src/engine/control/XButton.js");
const { XScroll } = __webpack_require__(/*! ../engine/control/XScroll */ "./src/engine/control/XScroll.js");
const { XSelect } = __webpack_require__(/*! ../engine/control/XSelect */ "./src/engine/control/XSelect.js");
const { X2DFast } = __webpack_require__(/*! ../engine/graphics/X2DFast */ "./src/engine/graphics/X2DFast.js");
const { XTexture } = __webpack_require__(/*! ../engine/graphics/XTexture */ "./src/engine/graphics/XTexture.js");
const { CanvasInput } = __webpack_require__(/*! ./CanvasInput */ "./src/ir/CanvasInput.js");
const { IrToPicture } = __webpack_require__(/*! ./IrToPicture */ "./src/ir/IrToPicture.js");
const { LogParser } = __webpack_require__(/*! ./LogParser */ "./src/ir/LogParser.js");
const { NapiLog } = __webpack_require__(/*! ./NapiLog */ "./src/ir/NapiLog.js");

const INTYPE_STR = ["state", "depend", "value", "framestate", "root", "other"];

class IrViewer {
  constructor(fn, result) {
    this.t1_ = Date.now();
    this.fileName_ = fn;
    this.parser_ = new LogParser(result);
    this.inited_ = false;
  }
  InitViewer(result) {
    this.data_ = result;
    this.direct_ = null;
    this.selectPoint_ = [];
    this.visable_ = null;

    this.loaded_ = false;

    this.offx_ = Scr.logicw / 2;
    this.offy_ = 30;

    let tx = 10;
    let ty = 10;
    let files = Object.keys(this.data_);
    this.selectFile_ = new XSelect(files, files[0]);
    this.selectFile_.move(tx, ty, 550, 20);
    this.selectFile_.registCallback(this.changeFile.bind(this));

    tx = 10;
    ty += 30;
    this.selectFunc_ = new XSelect([], "");
    this.selectFunc_.move(tx, ty, 290, 20);
    this.selectFunc_.registCallback(this.changeFunc.bind(this));

    tx += 290 + 10;
    this.selectMethod_ = new XSelect([], "");
    this.selectMethod_.move(tx, ty, 250, 20);
    this.selectMethod_.registCallback(this.changeMethod.bind(this));

    tx = 10;
    ty += 30;
    this.btnGo_ = [];
    this.mask_ = 0xffff;
    for (let i = 0; i < INTYPE_STR.length; i++) {
      let bname = INTYPE_STR[i] + "✔️";//❌
      let bw = X2DFast.gi().getTextWidth(bname, 14) + 6;
      let btn = new XButton(tx, ty, bw, 20, bname);
      btn.inTypeId_ = i;
      btn.inTypeMask_ = 1;
      btn.onClicked_ = () => {
        btn.inTypeMask_ = 1 - btn.inTypeMask_;
        btn.name_ = INTYPE_STR[btn.inTypeId_] + (btn.inTypeMask_ == 1 ? "✔️" : "❌");
        this.mask_ = (this.mask_ & ~(1 << btn.inTypeId_)) | (btn.inTypeMask_ << btn.inTypeId_);
        this.changeVisable();
      }
      this.btnGo_.push(btn);
      tx += bw + 10;
    }

    tx = 10;
    ty += 30;
    let bms = [["隐藏选中", () => { this.hideNode(0); }],
    ["隐藏未选中", () => { this.hideNode(1); }],
    ["显示隐藏", () => { this.hideNode(2); }],
    ["选中前继", () => { this.selectNode(0); }],
    ["选中后继", () => { this.selectNode(1); }],
    ["刷新", () => { this.freshNode(); }]];
    for (let bm of bms) {
      let bw = X2DFast.gi().getTextWidth(bm[0], 14) + 6;
      let btn = new XButton(tx, ty, bw, 20, bm[0]);
      btn.onClicked_ = bm[1];
      this.btnGo_.push(btn);
      tx += bw + 10;
    }

    this.btnGo_.push(this.selectFile_, this.selectFunc_, this.selectMethod_);
    this.btnGo_.sort((a, b) => {
      return b.posY_ - a.posY_;
    })

    this.scrollY_ = new XScroll({ type: "right" });
    this.scrollX_ = new XScroll({ type: "button" });
    this.scrollY_.move(Scr.logicw - 20, 100, 20, Scr.logich - 100 - 20);
    this.scrollX_.move(20, Scr.logich - 20, Scr.logicw - 40, 20);

    this.hideNodeIds_ = [];
    this.pointFile_ = files[0];
  }
  freshNode() {
    this.scrollY_.setBarOff(0);
    this.scrollX_.setBarOff(0.5);
    IrToPicture.resetPicture(this.visable_.nodes, this.direct_.type.startsWith("block:"));
  }
  hideNode(type) {
    if (type == 0) {//隐藏选中
      this.hideNodeIds_ = this.hideNodeIds_.concat(this.selectPoint_);
    }
    else if (type == 1) {//隐藏未选中
      let nodes = this.visable_.nodes;
      for (let k in nodes) {
        if (this.selectPoint_.indexOf(parseInt(nodes[k].ir.id)) >= 0) {
          continue;
        }
        this.hideNodeIds_.push(parseInt(nodes[k].ir.id));
      }
    }
    else {//显示所有
      this.hideNodeIds_ = [];
    }
    this.changeVisable();
  }
  selectNode(type) {
    let sel = new Set();
    let nodes = this.visable_.nodes;
    let lines = this.visable_.lines;
    let hideChanged = false;
    for (let l of lines) {
      let n1 = nodes[l.fromId];
      let n2 = nodes[l.toId];

      let id1 = parseInt(n1.ir.id);
      let id2 = parseInt(n2.ir.id);
      let idx = -1;
      if (type == 0 && (n1.mask & this.mask_) != 0 && this.selectPoint_.indexOf(id2) >= 0) {//选中前继
        idx = this.hideNodeIds_.indexOf(id1);
        sel.add(id1);
      }
      if (type == 1 && (n2.mask & this.mask_) != 0 && this.selectPoint_.indexOf(id1) >= 0) {//选中后继
        idx = this.hideNodeIds_.indexOf(id2);
        sel.add(id2);
      }
      if (idx >= 0) {
        this.hideNodeIds_.splice(idx, 1);
        hideChanged = true;
      }
    }
    this.selectPoint_ = [...sel];
    if (hideChanged) {
      this.changeVisable();
    }
  }
  loading() {
    if (this.loaded_) {
      return false;
    }
    if (this.parser_.parsing()) {
      return true;
    }
    if (!this.inited_) {
      this.inited_ = true;
      this.InitViewer(this.parser_.output_);
      return true;
    }
    let total = 1;
    let procto = 1;
    let loadonce = 1;
    for (let file in this.data_) {
      for (let func in this.data_[file]) {
        for (let method of this.data_[file][func]) {
          total++;
          if (method.loaded) {
            procto++;
            continue;
          }
          if (loadonce <= 0) {
            continue;
          }
          loadonce--;

          method.irAll = IrToPicture.toPicture(method.irList, 1, method.type.startsWith("block:"));
          method.loaded = true;
        }
      }
    }
    if (loadonce == 0) {
      XTools.PROC_TO = 20 + procto / total * 80;
      return true;
    }
    XTools.PROC_TO = 100;
    this.loaded_ = true;
    this.changeFile(this.pointFile_);
    NapiLog.logInfo("load cost", Date.now() - this.t1_);
    return true;
  }
  changeFile(name) {
    this.pointFile_ = name;
    let funcs = Object.keys(this.data_[this.pointFile_]);
    this.selectFunc_.resetList(funcs, funcs[0]);
    this.changeFunc(funcs[0]);
  }
  changeFunc(name) {
    this.pointFunc_ = name;
    let methods = [];
    for (let i = 0; i < this.data_[this.pointFile_][this.pointFunc_].length; i++) {
      methods.push((i + 1) + "," + this.data_[this.pointFile_][this.pointFunc_][i].type);
    }
    this.selectMethod_.resetList(methods, methods[0]);
    this.changeMethod(methods[0]);
  }
  changeMethod(name) {
    this.pointMethod_ = name;
    let p = parseInt(name.split(",")[0]) - 1;
    this.direct_ = this.data_[this.pointFile_][this.pointFunc_][p];
    this.changeVisable();
  }
  changeVisable() {
    this.visable_ = this.direct_.irAll;
    let nodes = this.visable_.nodes;
    let lines = this.visable_.lines;

    let showNodes = [];
    for (let k in nodes) {
      let n = nodes[k];
      if (this.hideNodeIds_.indexOf(parseInt(n.ir.id)) >= 0) {
        n.hide = true;
      }
      else {
        n.hide = (n.mask & this.mask_) == 0;
        if (!n.hide) {
          showNodes.push(k);
        }
      }
    }
    for (let k of showNodes) {
      let n = nodes[k];
      for (let i = 0; i < 5; i++) {
        if ((this.mask_ & (1 << i) != 0) && (n.mask & (1 << i) != 0)) {//进入点也加进来
          for (let id of n.ir.in[i]) {
            nodes[id].hide = false;
          }
        }
      }
    }
    for (let k in nodes) {
      let n = nodes[k];
      if (this.hideNodeIds_.indexOf(parseInt(n.ir.id)) >= 0) {
        n.hide = true;
      }
    }
    this.scrollY_.setBarOff(0);
    this.scrollX_.setBarOff(0.5);
  }
  makeLevely(nodes) {
    let levely = new Set();
    for (let k in nodes) {
      let n = nodes[k];
      if (n.hide) {
        continue;
      }
      if (n.deep != IrToPicture.INVALID_DEEP) {
        levely.add(n.pos.y);
      }
    }
    return Array.from(levely).sort((a, b) => { return parseInt(a) - parseInt(b) });
  }
  drawSmallMap(nodes, x1, x2, y1, y2) {
    if (x1 == x2 || y2 == y1) {
      return;
    }
    let [tx, ty, w, h] = this.smallMapRect;
    X2DFast.gi().fillRect(tx, ty, w, h, 0x80000000);

    let sw = w / (x2 - x1);
    let sh = h / (y2 - y1);

    let dh = Math.max(20 * sh, 1);
    for (let k in nodes) {//画节点
      let n = nodes[k];
      if (n.hide) {
        continue;
      }
      let dx = n.pos.x - x1;
      let dy = n.pos.y - y1;
      let dw = Math.max((n.nameWidth + 6) * sw, 1);
      if (this.selectPoint_.indexOf(parseInt(k)) >= 0) {
        if (this.drapSelect_) {
          dx += this.drapSelect_.dx;
          dy += this.drapSelect_.dy;
        }
        X2DFast.gi().fillRect(tx + (dx - 3) * sw, ty + (dy - 10) * sh, dw, dh, 0xff000000);
      }
      else {
        let selectWith = false;
        for (let inl of n.in) {
          if (this.selectPoint_.indexOf(parseInt(inl.fromId)) >= 0) {
            selectWith = true;
            break;
          }
        }
        if (!selectWith) {
          for (let outl of n.out) {
            if (this.selectPoint_.indexOf(parseInt(outl.toId)) >= 0) {
              selectWith = true;
              break;
            }
          }
        }
        if (selectWith) {
          X2DFast.gi().fillRect(tx + (dx - 3) * sw, ty + (dy - 10) * sh, dw, dh, 0xff000000);
        }
        else {
          X2DFast.gi().fillRect(tx + (dx - 3) * sw, ty + (dy - 10) * sh, dw, dh, XTools.CONFIG.NodeColor[n.type]);
        }
      }
    }
    X2DFast.gi().drawRect(tx - (this.offx_ + x1) * sw, ty - (this.offy_ + y1) * sh, Math.min(Scr.logicw * sw, w), Math.min(Scr.logich * sh, h), 0xff00ff00, 1);
  }
  onDraw() {
    if (this.loading()) {
      X2DFast.gi().drawText("Loading " + XTools.PROC_TO.toFixed(1) + "%", 20, Scr.logicw / 2, Scr.logich / 2, 1, 1, 0, -2, -2, 0xff000000);
      return;
    }
    let smallMapSize = parseInt(Math.min(Scr.logicw / 3, Scr.logich / 3));
    this.smallMapRect = [Scr.logicw - 50 - smallMapSize, 50, smallMapSize, smallMapSize];
    let nodes = this.visable_.nodes;
    let lines = this.visable_.lines;
    let levely = this.makeLevely(nodes);
    let maxx = -9999;
    let minx = 9999;
    let mouseOn = -1;
    let collect = {
      singleCount: 0,
      showCount: 0,
      nodeCount: Object.keys(nodes).length,
    };
    for (let k in nodes) {
      let n = nodes[k];
      if (n.hide) {
        continue;
      }
      collect.showCount++;
      if (n.deep != IrToPicture.INVALID_DEEP) {
        collect.singleCount++;
        if (maxx < n.pos.x + n.nameWidth + this.offx_) {
          maxx = n.pos.x + n.nameWidth + this.offx_;
        }
        if (minx > n.pos.x + this.offx_) {
          minx = n.pos.x + this.offx_;
        }
      }
      if (XTools.InRect(XTools.MOUSE_POS.x, XTools.MOUSE_POS.y, n.pos.x + this.offx_ - 3, n.pos.y + this.offy_ - 10, n.nameWidth + 6, 20)) {
        mouseOn = k;
      }
      n.outhx = {};
    }
    this.selectLines_ = [];
    let mmx1 = this.drawLines(this.offx_, this.offy_, nodes, lines, levely, [minx - 20, maxx + 20], false);//未选中的线
    for (let k in nodes) {//画节点
      let n = nodes[k];
      if (n.deep == IrToPicture.INVALID_DEEP) {
        if (n.pos.x == IrToPicture.INVALID_DEEP) {
          n.pos.x = mmx1[1] - this.offx_ + 20;
        }
      }
      if (n.hide) {
        continue;
      }
      let dx = n.pos.x + this.offx_;
      let dy = n.pos.y + this.offy_;
      if (this.selectPoint_.indexOf(parseInt(k)) >= 0) {
        if (this.drapSelect_) {
          dx += this.drapSelect_.dx;
          dy += this.drapSelect_.dy;
        }
        X2DFast.gi().fillRect(dx - 3, dy - 10, n.nameWidth + 6, 20, 0xffffff00);
        X2DFast.gi().drawRect(dx - 3, dy - 10, n.nameWidth + 6, 20, 0xff000000, 2);
      }
      else {
        X2DFast.gi().fillRect(dx - 3, dy - 10, n.nameWidth + 6, 20, XTools.CONFIG.NodeColor[n.type]);
        let selectWith = false;
        for (let inl of n.in) {
          if (this.selectPoint_.indexOf(parseInt(inl.fromId)) >= 0) {
            selectWith = true;
            break;
          }
        }
        if (!selectWith) {
          for (let outl of n.out) {
            if (this.selectPoint_.indexOf(parseInt(outl.toId)) >= 0) {
              selectWith = true;
              break;
            }
          }
        }
        if (selectWith) {
          X2DFast.gi().drawRect(dx - 3, dy - 10, n.nameWidth + 6, 20, 0xff000000, 2);
        }
      }
      X2DFast.gi().drawText(n.name, 14, dx + n.nameWidth / 2, dy + 2, 1, 1, 0, -2, -2, 0xff000000);
    }
    this.drawLines(this.offx_, this.offy_, nodes, lines, levely, [minx - 20, maxx + 20], true);//选中的线
    for (let ln of this.selectLines_) {
      let [r, g, b, a] = XTexture.ExpandColor(ln[4]);
      r = Math.max(0, r * 255 - 32);
      g = Math.max(0, g * 255 - 32);
      b = Math.max(0, b * 255 - 32);
      this.drawLine(ln[0], ln[1], ln[2], ln[3], 0xff000000 | (r << 16) | (g << 8) | b, ln[5] + 1);
    }
    if (mouseOn >= 0) {
      let n = nodes[mouseOn];//显示选中节点的信息
      let w = n.ir.maxDetailWidth + 2;
      let h = n.ir.detailList.length * 16 + 2;
      let x = XTools.MOUSE_POS.x - w;
      let y = XTools.MOUSE_POS.y - h;
      if (x < 10) {
        x = 10;
      }
      if (y < 130) {
        y = 130;
      }

      X2DFast.gi().fillRect(x, y, w, h, (XTools.CONFIG.NodeColor[n.type] & 0xffffff) | 0xC0000000);

      for (let i = 0; i < n.ir.detailList.length; i++) {
        X2DFast.gi().drawText(n.ir.detailList[i], 14, x + 1, y + 1 + i * 16, 1, 1, 0, -1, -1, 0xff000000);
      }
    }

    for (let btn of this.btnGo_) {
      btn.draw();
    }

    let x1 = 9999;
    let y1 = 9999;
    let x2 = -9999;
    let y2 = -9999;
    for (let k in nodes) {
      let n = nodes[k];
      if (n.hide) {
        continue;
      }
      if (n.pos.x < x1)
        x1 = n.pos.x;
      if (n.pos.x + n.nameWidth > x2)
        x2 = n.pos.x + n.nameWidth;

      if (n.pos.y < y1)
        y1 = n.pos.y;
      if (n.pos.y + n.nameWidth > y2)
        y2 = n.pos.y + IrToPicture.NODEH;
    }
    x1 = Math.min(mmx1[0] - this.offx_, x1) - Scr.logicw / 3;
    x2 = Math.max(mmx1[1] - this.offx_, x2) + Scr.logicw / 3;
    y1 = y1 - Scr.logich / 3;
    y2 = y2 + Scr.logich / 3;
    this.dragScoll = {
      x1: x1,
      x2: x2,
      y1: y1,
      y2: y2,
    }
    let scrollW = x2 - x1;
    let scrollH = y2 - y1;
    this.dragScoll.hh = scrollH - Scr.logich;
    this.dragScoll.ww = scrollW - Scr.logicw;
    if (this.dragScoll.hh < 1) this.dragScoll.hh = 1;
    if (this.dragScoll.ww < 1) this.dragScoll.ww = 1;
    if (this.drapBackground_) {
      this.scrollY_.setBarOff(-(this.offy_ + this.dragScoll.y1) / this.dragScoll.hh);
      this.scrollX_.setBarOff(-(this.offx_ + this.dragScoll.x1) / this.dragScoll.ww);
    }
    else {
      this.offy_ = (-this.scrollY_.getBarOff()) * this.dragScoll.hh - this.dragScoll.y1;
      this.offx_ = (-this.scrollX_.getBarOff()) * this.dragScoll.ww - this.dragScoll.x1;
    }
    if (this.dragScoll.hh > 1) this.scrollY_.move(Scr.logicw - 20, 100, 20, Scr.logich - 100 - 20).draw();
    if (this.dragScoll.ww > 1) this.scrollX_.move(20, Scr.logich - 20, Scr.logicw - 40, 20).draw();

    this.drawSmallMap(nodes, x1, x2, y1, y2);

    if (this.searchInput) {
      let x = this.searchInput.pos[0];
      let y = this.searchInput.pos[1];
      let w = this.searchInput.pos[2];
      let h = this.searchInput.pos[3];
      X2DFast.gi().fillRect(x, y, w, h, 0x80000000);

      let searchResultTxt =
        this.searchInput.result.length == 0
          ? '0/0'
          : this.searchInput.point + 1 + '/' + this.searchInput.result.length;

      this.searchInput.btnUp.move(x + 20, y + 50, 32, 24).draw();

      X2DFast.gi().drawText(
        searchResultTxt,
        20,
        x + w / 2,
        y + 50 + 12,
        1,
        1,
        0,
        -2,
        -2,
        0xffffffff
      ) + 16;

      this.searchInput.btnDown.move(x + w - 20 - 32, y + 50, 32, 24).draw();
      this.searchInput.btnClose.move(x + w - 40, y + 10, 30, 30).draw();
    }
  }
  checkLevel(levely, n1, n2) {
    let i1 = levely.indexOf(n1.pos.y);
    let i2 = levely.indexOf(n2.pos.y);
    return i1 + 1 == i2;
  }
  drawLines(offx, offy, nodes, lines, levely, mmx, select) {
    let aaa = 5;
    if (true) {
      aaa = -5;
      for (let l of lines) {
        let n1 = nodes[l.fromId];
        let n2 = nodes[l.toId];
        if (n1.hide || n2.hide) {
          continue;
        }

        let lor = n1.pos.x + n2.pos.x < -50 ? 0 : 1;
        if (this.checkLevel(levely, n1, n2)) { }
        else {
          if (!(n1.outh[l.outNum] in n1.outhx)) {
            mmx[lor] += lor == 0 ? aaa : -aaa;
            n1.outhx[n1.outh[l.outNum]] = mmx[lor];
          }
        }
      }
    }
    let mmx1 = [mmx[0], mmx[1]];
    for (let l of lines) {
      let n1 = nodes[l.fromId];
      let n2 = nodes[l.toId];
      if (n1.hide || n2.hide) {
        continue;
      }

      let x1 = n1.pos.x + n1.nameWidth - 5 + offx - n1.ltypes.indexOf(l.lineType) * 5;
      let y1 = n1.pos.y + 10 + offy;
      let x2 = n2.pos.x + n2.nameWidth - 5 + offx - l.inNum * 5;
      let y2 = n2.pos.y - 10 + offy;
      let lor = n1.pos.x + n2.pos.x < -50 ? 0 : 1;

      let selected = false;
      if (this.selectPoint_.indexOf(l.fromId) >= 0 || this.selectPoint_.indexOf(l.toId) >= 0) {
        selected = true;
        if (this.drapSelect_) {
          if (this.selectPoint_.indexOf(l.fromId) >= 0) {
            x1 += this.drapSelect_.dx;
            y1 += this.drapSelect_.dy;
          }
          if (this.selectPoint_.indexOf(l.toId) >= 0) {
            x2 += this.drapSelect_.dx;
            y2 += this.drapSelect_.dy;
          }
        }
      }

      if (select != selected) {
        if (this.checkLevel(levely, n1, n2)) { }
        else {
          mmx[lor] += lor == 0 ? -aaa : aaa;
        }
        continue;
      }

      let c = 0xffc0c0c0;
      let lw = 1;

      if (selected) {//选中的点进出的线使用指定的颜色，增加线宽
        c = XTools.CONFIG.LineColor[l.lineType];
        lw = 2;
      }
      let ls = [];
      if (this.checkLevel(levely, n1, n2)) {
        ls.push([x1, y1, x1, y1 + n1.outh[l.outNum], c, lw]);
        ls.push([x1, y1 + n1.outh[l.outNum], x2, y1 + n1.outh[l.outNum], c, lw]);
        ls.push([x2, y1 + n1.outh[l.outNum], x2, y2, c, lw]);
      }
      else {
        let lx = n1.outhx[n1.outh[l.outNum]];//n1.outhx[l.outNum] 或 mmx[lor]
        let ly = n2.inh[l.fromId + l.lineType];//n2.inh[l.inNum] 或 n2.inh[n1.ir.id]

        ls.push([x1, y1, x1, y1 + n1.outh[l.outNum], c, lw]);
        ls.push([x1, y1 + n1.outh[l.outNum], lx, y1 + n1.outh[l.outNum], c, lw]);
        ls.push([lx, y1 + n1.outh[l.outNum], lx, y2 - ly, c, lw]);
        ls.push([lx, y2 - ly, x2, y2 - ly, c, lw]);
        ls.push([x2, y2 - ly, x2, y2, c, lw]);
        mmx[lor] += lor == 0 ? -aaa : aaa;
      }
      let mouseOn = false;
      for (let ln of ls) {
        mouseOn |= this.drawLine(...ln);
      }
      if (mouseOn) {
        this.selectLines_.push(...ls);
      }
    }
    return [Math.min(mmx1[0], mmx[0]), Math.max(mmx1[1], mmx[1])];
  }
  drawLine(x1, y1, x2, y2, c, lw = 1) {
    if (x1 == x2) {
      if (y1 > y2) {
        [y1, y2] = [y2, y1];
      }
      X2DFast.px2f.fillRect(x1, y1, lw, y2 - y1 + lw, c);
      if (XTools.InRect(XTools.MOUSE_POS.x, XTools.MOUSE_POS.y, x1 - 1, y1, lw + 2, y2 - y1)) {
        return true;
      }
    }
    else if (y1 == y2) {
      if (x1 > x2) {
        [x1, x2] = [x2, x1];
      }
      X2DFast.px2f.fillRect(x1, y1, x2 - x1, lw, c);
      if (XTools.InRect(XTools.MOUSE_POS.x, XTools.MOUSE_POS.y, x1, y1 - 1, x2 - x1, lw + 2)) {
        return true;
      }
    }
    else {

    }
    return false;
  }
  locateNode(p) {
    this.selectPoint_ = [parseInt(p)];
    let nodes = this.visable_.nodes;
    let n = nodes[p];

    this.offx_ = Scr.logicw / 2 - n.pos.x;
    this.offy_ = Scr.logich / 2 - n.pos.y;
    this.scrollY_.setBarOff(-(this.offy_ + this.dragScoll.y1) / this.dragScoll.hh);
    this.scrollX_.setBarOff(-(this.offx_ + this.dragScoll.x1) / this.dragScoll.ww);
    this.offy_ = (-this.scrollY_.getBarOff()) * this.dragScoll.hh - this.dragScoll.y1;
    this.offx_ = (-this.scrollX_.getBarOff()) * this.dragScoll.ww - this.dragScoll.x1;
  }
  findNext() {
    if (this.searchInput) {
      this.searchInput.point += 1;
      if (this.searchInput.point >= this.searchInput.result.length) {
        this.searchInput.point = 0;
      }
      this.locateNode(this.searchInput.result[this.searchInput.point]);
    }
  }
  resetOffset(x, y) {
    let [tx, ty, w, h] = this.smallMapRect;
    let [x1, y1, x2, y2] = [this.dragScoll.x1, this.dragScoll.y1, this.dragScoll.x2, this.dragScoll.y2];
    if (x1 == x2 || y1 == y2) {
      return;
    }
    let sw = w / (x2 - x1);
    let sh = h / (y2 - y1);
    this.offx_ = (tx - x + Scr.logicw * sw / 2) / sw - x1;
    this.offy_ = (ty - y + Scr.logich * sh / 2) / sh - y1;
    this.scrollY_.setBarOff(-(this.offy_ + this.dragScoll.y1) / this.dragScoll.hh);
    this.scrollX_.setBarOff(-(this.offx_ + this.dragScoll.x1) / this.dragScoll.ww);
    this.offy_ = (-this.scrollY_.getBarOff()) * this.dragScoll.hh - this.dragScoll.y1;
    this.offx_ = (-this.scrollX_.getBarOff()) * this.dragScoll.ww - this.dragScoll.x1;
  }
  onTouch(msg, x, y) {
    if (this.loading()) {
      return true;
    }
    if (this.smallMapLocked_) {
      if (msg == 2) {
        this.resetOffset(x, y);
      }
      if (msg == 3) {
        this.smallMapLocked_ = false;
      }
      return true;
    }
    if (msg == 6) {
      this.drapBackground_ = null;
    }
    if (msg == 3 && this.drapSelect_) {
      let nodes = this.visable_.nodes;
      for (let k of this.selectPoint_) {
        nodes[k].pos.x += this.drapSelect_.dx;
        nodes[k].pos.y += this.drapSelect_.dy;
      }
      this.drapSelect_ = null;
    }
    if (this.drapBackground_) {
      if (msg == 2) {
        this.offx_ -= this.drapBackground_.x - x;
        this.offy_ -= this.drapBackground_.y - y;
        this.drapBackground_.x = x;
        this.drapBackground_.y = y;
      }
      return true;
    }
    if (this.drapSelect_) {
      if (msg == 2) {
        if (Math.abs(this.drapSelect_.x - x) > 10 ||
          Math.abs(this.drapSelect_.y - y) > 10 ||
          this.drapSelect_.dx != 0 ||
          this.drapSelect_.dy != 0) {
          this.drapSelect_.dx -= this.drapSelect_.x - x;
          this.drapSelect_.dy -= this.drapSelect_.y - y;
          this.drapSelect_.x = x;
          this.drapSelect_.y = y;
        }
      }
      return true;
    }
    if (this.scrollX_.onTouch(msg, x, y)) {
      return true;
    }
    if (this.scrollY_.onTouch(msg, x, y)) {
      return true;
    }
    if (XTools.InRect(x, y, ...this.smallMapRect)) {
      if (msg == 1) {
        this.resetOffset(x, y);
        this.smallMapLocked_ = true;
      }
      return true;
    }
    if (this.searchInput) {
      if (XTools.InRect(x, y, ...this.searchInput.pos)) {
        if (this.searchInput.btnUp.onTouch(msg, x, y)) {
          if (this.searchInput.btnUp.isClicked()) {
            this.searchInput.point -= 1;
            if (this.searchInput.point < 0) {
              this.searchInput.point = this.searchInput.result.length - 1;
            }
            this.locateNode(this.searchInput.result[this.searchInput.point]);
          }
        }
        if (this.searchInput.btnDown.onTouch(msg, x, y)) {
          if (this.searchInput.btnDown.isClicked()) {
            this.findNext();
          }
        }
        if (this.searchInput.btnClose.onTouch(msg, x, y)) {
          if (this.searchInput.btnClose.isClicked()) {
            this.searchInput = null;
            CanvasInput.HideEx();
          }
        }
        return true;
      }
    }
    for (let i = this.btnGo_.length - 1; i >= 0; i--) {
      if (this.btnGo_[i].onTouch(msg, x, y)) {
        return true;
      }
    }
    if (msg == 1) {
      let nodes = this.visable_.nodes;
      for (let k in nodes) {
        let n = nodes[k];
        if (n.hide) {
          continue;
        }
        if (XTools.InRect(x, y, n.pos.x + this.offx_ - 3, n.pos.y + this.offy_ - 10, n.nameWidth + 6, 20)) {
          if (XTools.KEY_CTRL) {
            this.selectPoint_.push(parseInt(k));
          }
          else {
            if (this.selectPoint_.indexOf(parseInt(k)) < 0) {
              this.selectPoint_ = [parseInt(k)];
            }
          }
          this.drapSelect_ = {
            x: x,
            y: y,
            dx: 0,
            dy: 0,
          }
          return true;
        }
      }
      this.selectPoint_ = [];
    }

    if (msg == 4) {
      this.drapBackground_ = {
        x: x,
        y: y,
      }
    }
    return false;
  }
  onKey(k) {
    if (this.loading()) {
      return true;
    }
    if (this.searchInput) {
      return true;
    }
    switch (k) {
      case "PageUp":
        this.selectNode(0);
        return true;
      case "PageDown":
        this.selectNode(1);
        return true;
      case "H":
      case "h":
        this.hideNode(0);
        return true;
      case " ":
        this.hideNode(1);
        return true;
      case "S":
      case "s":
        this.hideNode(2);
        return true;
      case "Enter":
        this.freshNode();
        return true;
    }
    if (k == 'ctrl+f' || k == 'ctrl+F') {
      this.searchInput = {
        pos: [(Scr.logicw - 300), Scr.logich / 2, 200, 80],
        result: [],
        point: 0,
        btnUp: new XButton(0, 0, 0, 0, '<'),
        btnDown: new XButton(0, 0, 0, 0, '>'),
        btnClose: new XButton(0, 0, 0, 0, '❌'),
      };
      let x = this.searchInput.pos[0];
      let y = this.searchInput.pos[1];
      let w = this.searchInput.pos[2];
      let h = this.searchInput.pos[3];
      this.searchInput.Open = () => {
        CanvasInput.Reset(x, y + 10, w - 32 - 40, 32, '', null, (v) => {
          function isRegExp(s) {
            try {
              new RegExp(s);
              return true;
            } catch (e) {
              return false;
            }
          }
          this.searchInput.result = [];
          if (v.length > 0) {
            let nodes = this.visable_.nodes;
            this.selectPoint_ = [];
            for (let i in nodes) {
              let n = nodes[i];
              if (n.ir.op == "JS_BYTECODE") {
                if (n.ir.id == v || n.ir.bytecode.indexOf(v) >= 0 || (isRegExp(v) && n.ir.bytecode.match(v))) {
                  this.searchInput.result.push(i);
                }
              }
              else {
                if (n.ir.id == v || n.ir.op.indexOf(v) >= 0 || (isRegExp(v) && n.ir.op.match(v))) {
                  this.searchInput.result.push(i);
                }
              }
            }
            if (this.searchInput.result.length > 0) {
              this.locateNode(this.searchInput.result[0]);
              this.searchInput.point = 0;
            }
          }
        }, this.findNext.bind(this));
      }
      CanvasInput.SetSafeArea(...this.searchInput.pos);
      this.searchInput.Open();
    }
    return false;
  }
}

module.exports = {
  IrViewer
}

/***/ }),

/***/ "./src/ir/LogParser.js":
/*!*****************************!*\
  !*** ./src/ir/LogParser.js ***!
  \*****************************/
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

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

const { XTools } = __webpack_require__(/*! ../engine/XTools */ "./src/engine/XTools.js");
const { X2DFast } = __webpack_require__(/*! ../engine/graphics/X2DFast */ "./src/engine/graphics/X2DFast.js");
const { NapiLog } = __webpack_require__(/*! ./NapiLog */ "./src/ir/NapiLog.js");

class LogParser {
  constructor(result) {
    const regexVT100 = /\x1B\[[0-9;]*[A-Za-z]/g; // 匹配VT100字符的正则表达式
    result = result.replace(regexVT100, '');
    result = result.replace(/\t/g, '    ');
    result = result.replace(/\r/g, '');
    this.logLines_ = result.split("\n");
    this.stat_ = 0;
    this.lineNum_ = 0;
    this.initBlock_ = {
      filePoint: null,
      funcPoint: null,
      blockType: -1,
    };
    this.procBlock_ = {
      blockStat: -1,
      blockCollect: -1,
      oneBlock: -1,
    }
    this.procNormal_ = null;
    this.output_ = {};
  }

  parsing() {
    if (this.lineNum_ >= this.logLines_.length) {
      return false;
    }

    for (let i = 0; i < 300 && this.lineNum_ < this.logLines_.length; i++) {
      this.parseLine(this.logLines_[this.lineNum_]);
      this.lineNum_++;
    }

    XTools.PROC_TO = this.lineNum_ * 20 / this.logLines_.length;
    return true;
  }
  isBlock(l) {
    const blockTypes = ["Build Basic Block",
      "Computed Dom Trees",
      "Inserted Phis",
      "Bytecode2Gate"];
    for (let bt of blockTypes) {
      if (l.indexOf(bt) >= 0) {
        this.stat_ = this.initBlock_.blockType;
        this.procBlock_.blockStat = 0;
        this.procBlock_.blockCollect = {
          type: "block:" + bt,
          func: this.initBlock_.funcPoint,
          file: this.initBlock_.filePoint,
          irList: [],
          startLine: l,
        }
        this.procBlock_.oneBlock = null;
        return true;
      }
    }
    return false;
  }
  splitLast(s) {
    let i = s.lastIndexOf("@");

    return [s.substring(0, i), s.substring(i + 1)];
  }
  isStart(l) {
    //========= After bytecode2circuit lowering [func_main_0@484@arkcompiler/ets_runtime/sd_test/ttabs.abc] ========
    const regexStart = /=+ *After ([a-zA-Z0-9_ ]+) \[([#a-zA-Z0-9_@/.]+)\] *=+/g
    //========= After inlining [OthreMath@test@arkcompiler/ets_runtime/sd_test/test.abc] Caller method [func_main_0@641@arkcompiler/ets_runtime/sd_test/test.abc]====================[0m
    const regexStart2 = /=+ *After ([a-zA-Z0-9_ ]+) \[([a-zA-Z0-9_@/.]+)\] *Caller method \[([#a-zA-Z0-9_@/.]+)\] *=+/g

    if (l[11] != '=') {
      return;
    }
    let ret = regexStart.exec(l);
    if (ret) {
      let tt = this.splitLast(ret[2]);
      this.procNormal_ = {
        type: ret[1],//优化类型
        func: tt[0],//函数名
        file: tt[1],//文件名
        irList: [],
        startLine: l,
      };
      this.stat_ = 1;
      [this.initBlock_.funcPoint, this.initBlock_.filePoint] = [tt[0], tt[tt.length - 1]];
      this.initBlock_.blockType = 10;
    }
    else {
      ret = regexStart2.exec(l);
      if (ret) {
        let tt = this.splitLast(ret[2]);
        let tt2 = this.splitLast(ret[3]);
        NapiLog.logInfo(tt[0], "Caller method(" + this.lineNum_ + "行)", ret[3]);
        this.procNormal_ = {
          type: ret[1] + " " + tt[0],//优化类型
          func: tt2[0],//函数名
          file: tt2[1],//文件名
          irList: [],
          startLine: l,
        };
        this.stat_ = 1;
        [this.initBlock_.funcPoint, this.initBlock_.filePoint] = [tt2[0], tt2[tt2.length - 1]];
        this.initBlock_.blockType = 10;
      }
      else {
        if (l.search("After") > 0) {
          alert(l);
        }
      }
    }
  }
  collectNormal(l) {
    let idx = l.search('{"id":');
    if (idx >= 0) {
      let idx2 = l.lastIndexOf('}');
      let str = l.substring(idx, idx2 + 1);

      let ir = JSON.parse(str);
      {//根据XTools.CONFIG.MTypeField切割MType
        let cutPos = [];
        for (let field of XTools.CONFIG.MTypeField) {
          let idx = ir.MType.indexOf(", " + field);
          if (idx >= 0) {
            cutPos.push(idx);
          }
        }
        cutPos.push(ir.MType.length);
        cutPos.sort((a, b) => { return parseInt(a) - parseInt(b) });
        if (cutPos[0] == 0) {
          cutPos.shift();
        }
        let p = 0;
        let cutResult = [];
        for (let i of cutPos) {
          let tmp = ir.MType.substring(p, i);
          if (tmp.startsWith(", ")) {
            tmp = tmp.substring(2);
          }
          if (tmp.endsWith(", ")) {
            tmp = tmp.substring(0, tmp.length - 2);
          }
          cutResult.push(tmp);
          p = i;
        }
        cutResult.push("inNum=[" + ir.in[0].length + "," + ir.in[1].length + "," + ir.in[2].length + "," + ir.in[3].length + "," + ir.in[4].length + "]")
        cutResult.push("outNum=" + ir.out.length);
        ir.maxDetailWidth = 0;
        for (let detail of cutResult) {
          let w = X2DFast.gi().getTextWidth(detail, 14);
          if (ir.maxDetailWidth < w) {
            ir.maxDetailWidth = w;
          }
        }
        ir.detailList = cutResult;
      }
      this.procNormal_.irList.push(ir);
    }
    else {
      //= End typeHitRate: 0.500000 =
      let regexEnd = /=+ End[a-zA-Z:.0-9 ]* =+/g
      let tt = regexEnd.exec(l);
      if (tt) {//收集结束，入大表l.search('== End ==') > 0
        if (this.procNormal_.irList.length > 0) {
          if (!(this.procNormal_.file in this.output_)) {
            this.output_[this.procNormal_.file] = {};
          }
          if (!(this.procNormal_.func in this.output_[this.procNormal_.file])) {
            this.output_[this.procNormal_.file][this.procNormal_.func] = [];
          }
          this.output_[this.procNormal_.file][this.procNormal_.func].push(this.procNormal_);
        }
        else {
          NapiLog.logError("After和End之间没有指令(" + this.lineNum_ + "行)");
        }
        this.stat_ = 0;
      }
      else {
        NapiLog.logError("After和End之间解析失败(" + (this.lineNum_ + 1) + ")行");
        this.stat_ = 0;
      }
    }
  }
  parseLine(l) {
    switch (this.stat_) {
      case 0://搜索起始
        if (this.SearchBegin(l) || this.isBlock(l)) {
          return;
        }
        this.isStart(l);
        break;
      case 1://收集ir表
        this.collectNormal(l);
        break;
      case 10://收集block一
        if (this.CollectBlock(l)) {
          this.stat_ = 0;
          this.lineNum_ -= 1;
        }
        break;
      case 20://收集block二
        if (this.CollectBlock2(l)) {
          this.stat_ = 0;
          this.lineNum_ -= 1;
        }
        break;
    }
  }

  static Load(fn, cb) {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', fn);
    xhr.onreadystatechange = () => {
      if (xhr.readyState === XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          XTools.PORC_TO = 10;
          cb(fn, xhr.responseText);
        }
      }
    };
    xhr.send();
  }
  NumberStringToArray(ss) {
    let outs = ss.split(",");
    let ret = []
    for (let s of outs) {
      let ttt = parseInt(s);
      if (!isNaN(ttt)) {
        ret.push(ttt);
      }
    }
    return ret;
  }
  SearchBegin(l) {
    let ret;
    let ib = this.initBlock_;

    if (l.startsWith("[compiler] aot method")) {
      ////[compiler] aot method [func_main_0@b.abc] log:
      const regexFuncName = /^\[compiler\] aot method \[([#a-zA-Z0-9_@/.]+)\] (recordName \[[a-zA-Z0-9_]*\] )*log:/g
      ret = regexFuncName.exec(l);
      if (ret) {
        [ib.funcPoint, ib.filePoint] = this.splitLast(ret[1]);
        ib.blockType = 10;
        return true;
      }
    }
    if (l.startsWith("[compiler] ==================== Before state split")) {
      const regexFuncName2 = /^\[compiler\] =+ Before state split linearizer \[([#a-zA-Z0-9_@/.]+)\] *=*/g
      ret = regexFuncName2.exec(l);
      if (ret) {
        [ib.funcPoint, ib.filePoint] = this.splitLast(ret[1]);
        ib.blockType = 20;
        return true;
      }
    }
    if (l.startsWith("[compiler] ==================== After graph lineari")) {
      const regexFuncName3 = /^\[compiler\] =+ After graph linearizer \[([#a-zA-Z0-9_@/.]+)\] *=*/g
      ret = regexFuncName3.exec(l);
      if (ret) {
        [ib.funcPoint, ib.filePoint] = this.splitLast(ret[1]);
        ib.blockType = 20;
        return true;
      }
    }
    return false;
  }
  CollectBlock(l) {
    const regexBlock = [
      /^\[compiler\] B([0-9]+):                               ;preds=([0-9, ]*)$/g,//[compiler] B0:                               ;preds= 
      /^\[compiler\]  *Succes:([0-9, ]*)$/g,//[compiler] 	Succes: 
      /^\[compiler\]  *Bytecode\[\] = *(Empty)*$/g,//[compiler] 	Bytecode[] = Empty
      /^\[compiler\]  *Trys:([0-9, ]*)$/g,//[compiler] 	Trys: 
    ]
    let ret;
    let pb = this.procBlock_;
    if (pb.blockStat == 0) {
      ret = regexBlock[0].exec(l);
      if (ret) {
        pb.oneBlock = {
          id: ret[1],
          op: "B" + ret[1],
          detailList: [],
          maxDetailWidth: 0,
        }
        pb.oneBlock.in = [[], [], [], [], this.NumberStringToArray(ret[2])];
        return false;
      }
      if (!pb.oneBlock) {//完成了一个block的解析
        if (!(pb.blockCollect.file in this.output_)) {
          this.output_[pb.blockCollect.file] = {};
        }
        if (!(pb.blockCollect.func in this.output_[pb.blockCollect.file])) {
          this.output_[pb.blockCollect.file][pb.blockCollect.func] = [];
        }
        this.output_[pb.blockCollect.file][pb.blockCollect.func].push(pb.blockCollect);
        return true;
      }
      ret = regexBlock[1].exec(l);
      if (ret) {
        pb.oneBlock.out = this.NumberStringToArray(ret[1]);
        return false;
      }
      ret = regexBlock[2].exec(l);
      if (ret) {
        pb.blockStat = 1;
        return false;
      }
    }
    else if (pb.blockStat == 1) {//开始记录bytecode，直到空行，结束这个block
      if (/^\[compiler\] *$/g.test(l)) {//遇到空行，完成block
        if (pb.oneBlock.maxDetailWidth == 0) {
          pb.oneBlock.maxDetailWidth = X2DFast.gi().getTextWidth("Empty", 14);
          pb.oneBlock.detailList.push("Empty");
        }
        pb.blockCollect.irList.push(pb.oneBlock);
        pb.oneBlock = null;
        pb.blockStat = 0;
      }
      else {
        let s = l.substring(11);
        while (s.startsWith(" ")) {
          s = s.substring(1);
        }
        let w = X2DFast.gi().getTextWidth(s, 14);
        if (pb.oneBlock.maxDetailWidth < w) {
          pb.oneBlock.maxDetailWidth = w;
        }
        pb.oneBlock.detailList.push(s);
      }
    }
    return false;
  }
  CollectBlock2(l) {
    const regexBlock = [
      /^\[compiler\] B([0-9]+):/g,//[compiler] B0:                               ;preds= 
      /^\[compiler\]  *Preds:([0-9, ]*)$/g,
      /^\[compiler\]  *Succes:([0-9, ]*)$/g,//[compiler] 	Succes: 
      /^\[compiler\]  *Bytecode\[\] = *(Empty)*$/g,//[compiler] 	Bytecode[] = Empty
      /^\[compiler\]  *Trys:([0-9, ]*)$/g,//[compiler] 	Trys: 
    ]
    let pb = this.procBlock_;
    let ret;
    switch (pb.blockStat) {
      case 0:
        ret = regexBlock[0].exec(l);
        if (ret) {
          pb.oneBlock = {
            id: ret[1],
            op: "B" + ret[1],
            detailList: [],
            maxDetailWidth: 0,
          }
          pb.blockStat = 1;
          return false;
        }
        if (!pb.oneBlock) {//完成了一个block的解析
          if (!(pb.blockCollect.file in this.output_)) {
            this.output_[pb.blockCollect.file] = {};
          }
          if (!(pb.blockCollect.func in this.output_[pb.blockCollect.file])) {
            this.output_[pb.blockCollect.file][pb.blockCollect.func] = [];
          }
          this.output_[pb.blockCollect.file][pb.blockCollect.func].push(pb.blockCollect);
          return true;
        }
        break;
      case 1:
        ret = regexBlock[1].exec(l);
        if (ret) {
          pb.oneBlock.in = [[], [], [], [], this.NumberStringToArray(ret[1])];
          pb.blockStat = 2;
          return false;
        }
        break;
      case 2:
        ret = regexBlock[2].exec(l);
        if (ret) {
          pb.oneBlock.out = this.NumberStringToArray(ret[1]);
          pb.blockStat = 3;
          return false;
        }
        break;
      case 3://开始记录bytecode，直到空行，结束这个block
        if (/^\[compiler\] *$/g.test(l)) {//遇到空行，完成block
          if (pb.oneBlock.maxDetailWidth == 0) {
            pb.oneBlock.maxDetailWidth = X2DFast.gi().getTextWidth("Empty", 14);
            pb.oneBlock.detailList.push("Empty");
          }
          pb.blockCollect.irList.push(pb.oneBlock);
          pb.oneBlock = null;
          pb.blockStat = 0;
        }
        else {
          let s = l.substring(11);
          while (s.startsWith(" ")) {
            s = s.substring(1);
          }
          let w = X2DFast.gi().getTextWidth(s, 14);
          if (pb.oneBlock.maxDetailWidth < w) {
            pb.oneBlock.maxDetailWidth = w;
          }
          pb.oneBlock.detailList.push(s);
        }
        return false;
      default:
        return false;
    }
    return false;
  }
}

module.exports = {
  LogParser
}

/***/ }),

/***/ "./src/ir/NapiLog.js":
/*!***************************!*\
  !*** ./src/ir/NapiLog.js ***!
  \***************************/
/***/ ((module) => {

/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

class NapiLog {
  constructor() {}
}
NapiLog.LEV_NONE = 0;
NapiLog.LEV_ERROR = 1;
NapiLog.LEV_DEBUG = 2;
NapiLog.LEV_INFO = 3;

const LEV_STR = ['[NON]', '[ERR]', '[DBG]', '[INF]'];
var logLevel = NapiLog.LEV_INFO;
var logFileName = null;
var logResultMessage = [true, ''];
var errorCallBack = null;

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


/***/ })

/******/ 	});
/************************************************************************/
/******/ 	// The module cache
/******/ 	var __webpack_module_cache__ = {};
/******/ 	
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/ 		// Check if module is in cache
/******/ 		var cachedModule = __webpack_module_cache__[moduleId];
/******/ 		if (cachedModule !== undefined) {
/******/ 			return cachedModule.exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = __webpack_module_cache__[moduleId] = {
/******/ 			// no module.id needed
/******/ 			// no module.loaded needed
/******/ 			exports: {}
/******/ 		};
/******/ 	
/******/ 		// Execute the module function
/******/ 		__webpack_modules__[moduleId](module, module.exports, __webpack_require__);
/******/ 	
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/ 	
/************************************************************************/
/******/ 	/* webpack/runtime/compat get default export */
/******/ 	(() => {
/******/ 		// getDefaultExport function for compatibility with non-harmony modules
/******/ 		__webpack_require__.n = (module) => {
/******/ 			var getter = module && module.__esModule ?
/******/ 				() => (module['default']) :
/******/ 				() => (module);
/******/ 			__webpack_require__.d(getter, { a: getter });
/******/ 			return getter;
/******/ 		};
/******/ 	})();
/******/ 	
/******/ 	/* webpack/runtime/define property getters */
/******/ 	(() => {
/******/ 		// define getter functions for harmony exports
/******/ 		__webpack_require__.d = (exports, definition) => {
/******/ 			for(var key in definition) {
/******/ 				if(__webpack_require__.o(definition, key) && !__webpack_require__.o(exports, key)) {
/******/ 					Object.defineProperty(exports, key, { enumerable: true, get: definition[key] });
/******/ 				}
/******/ 			}
/******/ 		};
/******/ 	})();
/******/ 	
/******/ 	/* webpack/runtime/hasOwnProperty shorthand */
/******/ 	(() => {
/******/ 		__webpack_require__.o = (obj, prop) => (Object.prototype.hasOwnProperty.call(obj, prop))
/******/ 	})();
/******/ 	
/******/ 	/* webpack/runtime/make namespace object */
/******/ 	(() => {
/******/ 		// define __esModule on exports
/******/ 		__webpack_require__.r = (exports) => {
/******/ 			if(typeof Symbol !== 'undefined' && Symbol.toStringTag) {
/******/ 				Object.defineProperty(exports, Symbol.toStringTag, { value: 'Module' });
/******/ 			}
/******/ 			Object.defineProperty(exports, '__esModule', { value: true });
/******/ 		};
/******/ 	})();
/******/ 	
/************************************************************************/
var __webpack_exports__ = {};
// This entry need to be wrapped in an IIFE because it need to be in strict mode.
(() => {
"use strict";
/*!**********************!*\
  !*** ./src/index.js ***!
  \**********************/
__webpack_require__.r(__webpack_exports__);
/* harmony import */ var _engine_XDefine_js__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./engine/XDefine.js */ "./src/engine/XDefine.js");
/* harmony import */ var _engine_GLFrame_js__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./engine/GLFrame.js */ "./src/engine/GLFrame.js");
/* harmony import */ var _engine_graphics_X2DFast_js__WEBPACK_IMPORTED_MODULE_2__ = __webpack_require__(/*! ./engine/graphics/X2DFast.js */ "./src/engine/graphics/X2DFast.js");
/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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




const { NapiLog } = __webpack_require__(/*! ./ir/NapiLog */ "./src/ir/NapiLog.js");
const { MainEditor } = __webpack_require__(/*! ./MainEditor */ "./src/MainEditor.js");

let canvas = document.getElementById('visual_area');
const sideWidth = 0;//420;
canvas.width = window.innerWidth - sideWidth;
canvas.height = window.innerHeight;

function myDraw() {
  if (
    canvas.width != window.innerWidth - sideWidth ||
    canvas.height != window.innerHeight
  ) {
    canvas.width = window.innerWidth - sideWidth;
    canvas.height = window.innerHeight;

    _engine_XDefine_js__WEBPACK_IMPORTED_MODULE_0__.Scr.setLogicScreenSize(canvas.width, canvas.height);
    _engine_GLFrame_js__WEBPACK_IMPORTED_MODULE_1__.GLFrame.gi().resize();
  }

  let pm2f = _engine_graphics_X2DFast_js__WEBPACK_IMPORTED_MODULE_2__.X2DFast.gi();
  pm2f.swapMode2D();
  pm2f.clearBuffer();

  MainEditor.gi().onDraw(pm2f);

  pm2f.freshBuffer();
}

function myTouch(msg, x, y) {
  MainEditor.gi().onTouch(msg, x, y);
}

function myKey(type, code) {
  MainEditor.gi().onKey(code);
}

_engine_XDefine_js__WEBPACK_IMPORTED_MODULE_0__.Scr.setLogicScreenSize(canvas.width, canvas.height);
_engine_GLFrame_js__WEBPACK_IMPORTED_MODULE_1__.GLFrame.gi().go(canvas, myDraw, myTouch, myKey);

})();

/******/ })()
;
//# sourceMappingURL=main.js.map