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

import { CanvasInput } from '../ir/CanvasInput.js';
import { X2DFast } from './graphics/X2DFast.js';
import { Scr } from './XDefine.js';
import { XTools } from './XTools.js';

export var gl;
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
    XTools.KEY_CTRL = false;
  }
  if (!e.shiftKey) {
    XTools.KEY_SHIFT = false;
  }
  if (!e.altKey) {
    XTools.KEY_ALT = false;
  }
  e.preventDefault();
}

function keyDown(e) {
  let ret = '';
  if (e.ctrlKey) {
    if (ret.length > 0) ret += '+';
    ret += 'ctrl';
    XTools.KEY_CTRL = true;
  }
  if (e.shiftKey) {
    if (ret.length > 0) ret += '+';
    ret += 'shift';
    XTools.KEY_SHIFT = true;
  }
  if (e.altKey) {
    if (ret.length > 0) ret += '+';
    ret += 'alt';
    XTools.KEY_ALT = true;
  }
  if (ret.length > 0) ret += '+';
  ret += e.key;
  GLFrame.pinstance_.callbackKey(1, ret);
  if (!CanvasInput.FOCUS) {
  }
  if (ret === 'ctrl+z' || ret === 'ctrl+f' || ret === 'Enter') {
    e.preventDefault();
  }
}

function mainLoop() {
  GLFrame.pinstance_.callbackDraw();
  window.requestAnimationFrame(mainLoop);
}

export class GLFrame {
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
    cvs.addEventListener('mousewheel', mouseWheel);

    cvs.addEventListener('drop', (e) => {
      e.preventDefault();
      GLFrame.gi().callbackDropfile(e.dataTransfer.files, e.offsetX, e.offsetY);
    });
    cvs.addEventListener('dragover', (e) => {
      e.preventDefault();
    });
    cvs.addEventListener('dragenter', (e) => {
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
    if (this.pCallbackKey !== null) {
      this.pCallbackKey(type, code);
    }
  }
  callbackDraw() {
    gl.clearColor(1.0, 1.0, 1.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    if (this.pCallbackDraw !== null) {
      this.pCallbackDraw();
    }
  }

  callbackProctouch(msg, x, y) {
    XTools.MOUSE_POS.x = x;
    XTools.MOUSE_POS.y = y;
    if (this.pCallbackTouch !== null) {
      x = (x * Scr.logicw) / Scr.width;
      y = (y * Scr.logich) / Scr.height;
      this.pCallbackTouch(msg, x, y);
    }
  }
  callbackDropfile(files, x, y) {
    if (this.pCallbackDropfile !== null) {
      this.pCallbackDropfile(files, x, y);
    }
  }
  resize() {
    gl.viewport(0, 0, Scr.logicw, Scr.logich);
    X2DFast.gi().resetMat();
  }
}

GLFrame.pinstance_ = new GLFrame();
