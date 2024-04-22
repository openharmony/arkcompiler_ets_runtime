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

const { X2DFast } = require('../graphics/X2DFast');

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
    if (this.default_.indexOf('\\') !== -1) {
      let list = this.default_.split('\\');
      if (list.length > model) {
        for (let i = list.length - model; i < list.length; i++) {
          name += list[i];
          if (i !== list.length - 1) {
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
        if (i === this.tmpSelect_) {
          this.pm2f_.fillRect(x, y + h + i * 20, w, 20, this.backgroundColor_ + 0x303030);
        }

        let name1 = '...';
        if (this.list_[i].indexOf('\\') !== -1) {
          let list = this.list_[i].split('\\');
          if (list.length > model) {
            for (let k = list.length - model; k < list.length; k++) {
              name1 += list[k];
              if (k !== list.length - 1) {
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
        if (!isIn) {
          break;
        }
        if (!this.open_) {
          this.open_ = true;
          break;
        }
        if (this.tmpSelect_ >= 0 && this.tmpSelect_ <= this.list_.length) {
          if (this.default_ !== this.list_[this.tmpSelect_]) {
            this.default_ = this.list_[this.tmpSelect_];
            if (this.selectCallback !== null) {
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
