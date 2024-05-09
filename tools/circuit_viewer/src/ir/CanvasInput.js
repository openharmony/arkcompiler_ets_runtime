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
      if (e.key === 'Enter') {
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
