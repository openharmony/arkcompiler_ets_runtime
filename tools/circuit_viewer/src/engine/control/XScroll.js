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

const { X2DFast } = require('../graphics/X2DFast');

class XScroll {
  constructor(options) {
    if (options.type) {
      this.type_ = options.type;
    }
    else {
      this.type_ = 'right';
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
    if (this.type_ === 'right') {
      X2DFast.gi().fillRect(this.posX_ + 1, this.posY_ + this.barOff_, this.posW_ - 2, this.posH_ / 3, 0x40000000);
    }
    else if (this.type_ === 'button') {
      X2DFast.gi().fillRect(this.posX_ + this.barOff_, this.posY_ + 1, this.posW_ / 3, this.posH_ - 2, 0x40000000);
    }
  }
  isTouchIn(x, y) {
    if (x < this.posX_) {
      return false;
    }
    if (y < this.posY_) {
      return false;
    }
    if (x > this.posX_ + this.posW_) {
      return false;
    }
    if (y > this.posY_ + this.posH_) {
      return false;
    }
    return true;
  }
  setBarOff(rate) {
    if (this.type_ === 'right') {
      this.barOff_ = this.posH_ * 2 / 3 * rate;
    }
    else {
      this.barOff_ = this.posW_ * 2 / 3 * rate;
    }
    this.modifyBarOff(0, 0);
  }
  getBarOff() {
    if (this.type_ === 'right') {
      return this.barOff_ / (this.posH_ * 2 / 3);
    }
    else {
      return this.barOff_ / (this.posW_ * 2 / 3);
    }
  }
  modifyBarOff(dx, dy) {
    if (this.type_ === 'right') {
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
        if (this.type_ === 'right') {
          this.modifyBarOff(0, -this.posH_ / 3 / 10);
        }
        else if (isIn) {
          this.modifyBarOff(-this.posW_ / 3 / 10, 0);
        }
        break;
      case 11:
        if (this.type_ === 'right') {
          this.modifyBarOff(0, this.posH_ / 3 / 10);
        }
        else if (isIn) {
          this.modifyBarOff(this.posW_ / 3 / 10, 0);
        }
        break;
      case 1:
        if (isIn) {
          this.touchDown_ = true;
          if (this.type_ === 'right') {
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
          };
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
};