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

const { X2DFast } = require('./graphics/X2DFast');
const { Scr } = require('./XDefine');
const { XTools } = require('./XTools');

class RightMenu {
  static backgroundImg_ = -1;
  static backgroundCut_ = -1;

  static popItemFocusImg_ = -1;
  static popItemFocusCut_ = -1;
  static MENU = null;
  static PADDING = 16;
  static FOCUS_ITEM_MARGIN = 4;
  static FOCUS_ITEM_HEIGHT = 32;
  static TEXT_SIZE = 14;
  static isDarkBackground_ = true;
  static Reset(detail, x, y) {
    RightMenu.MENU = {
      x: x == null ? XTools.MOUSE_POS.x : x,
      y: y == null ? XTools.MOUSE_POS.y : y,
      detail: detail,
      needClose: false,
    };
  }
  static Append(detail) {
    if (RightMenu.MENU) {
      RightMenu.MENU.detail.push(...detail);
    }
  }
  static Button(icon, name, hootkey, callback) {
    return {
      type: 0,
      icon: icon,
      name: name,
      hk: hootkey,
      cb: () => {
        callback(this);
        RightMenu.close();
      },
    };
  }
  static Group(icon, name, objs) {
    return {
      type: 1,
      icon: icon,
      name: name,
      open: false,
      group: objs,
    };
  }
  static Gap() {
    return {
      type: 2,
    };
  }
  static close() {
    if (RightMenu.MENU) {
      RightMenu.MENU.needClose = true;
    }
  }
  static Draw() {
    if (RightMenu.MENU) {
      RightMenu.DrawGroup(
        RightMenu.MENU.detail,
        RightMenu.MENU.x,
        RightMenu.MENU.y
      );
      if (RightMenu.MENU.needClose) {
        RightMenu.MENU = null;
      }
    }
  }
  static MENUW = 155;
  static DrawGroup(grp, x, y) {
    let w = RightMenu.MENUW;
    let l = 0;
    for (let e of grp) {
      if (e.type != 2) {
        l += 1;
      }
    }
    if (grp.length == 3) {
      X2DFast.px2f.drawCut(this.backgroundCut_, x, y, 1, 0.88, 0, -1, -1);
    } else if (grp.length == 1) {
      X2DFast.px2f.drawCut(this.backgroundCut_, x, y, 1, 0.3, 0, -1, -1);
    } else {
      X2DFast.px2f.drawCut(this.backgroundCut_, x, y, 1, 1, 0, -1, -1);
    }
    for (let e of grp) {
      e.rect = [x, y, w, 32];
      if (e.on) {
        X2DFast.px2f.drawCut(
          this.popItemFocusCut_,
          x + RightMenu.FOCUS_ITEM_MARGIN,
          y
        );
      }
      if (e.type == 2) {
        e.rect = [x, y, w, 0];
        X2DFast.px2f.drawLine(x, y, x + w, y, 0xff808080, 2);
        continue;
      }
      let OFFY_ = y + RightMenu.FOCUS_ITEM_HEIGHT / 2 - RightMenu.TEXT_SIZE / 2;
      let textColor = this.isDarkBackground_ ? 0xffffffff : 0xff000000;
      X2DFast.px2f.drawText(
        e.name,
        RightMenu.TEXT_SIZE,
        x + RightMenu.PADDING,
        OFFY_,
        1,
        1,
        0,
        0,
        0,
        textColor
      );
      if (e.type == 0) {
        if (e.hk) {
          X2DFast.px2f.drawText(
            e.hk,
            RightMenu.TEXT_SIZE,
            x + w,
            OFFY_,
            1,
            1,
            0,
            -3,
            -2,
            0xff808080
          );
        }
      } else if (e.type == 1) {
        if (e.open) {
          X2DFast.px2f.drawText(
            '<',
            RightMenu.TEXT_SIZE,
            x + w,
            OFFY_,
            1,
            1,
            0,
            -3,
            -2,
            textColor
          );
          RightMenu.DrawGroup(e.group, x + w, y);
        } else {
          X2DFast.px2f.drawText(
            '>',
            RightMenu.TEXT_SIZE,
            x + w,
            OFFY_,
            1,
            1,
            0,
            -3,
            -2,
            textColor
          );
        }
      }
      y += 32;
    }
  }
  static Touch(msg, x, y) {
    if (RightMenu.MENU) {
      if (RightMenu.TouchGroup(RightMenu.MENU.detail, msg, x, y)) {
        return true;
      } else if (msg != 2) {
        RightMenu.MENU.needClose = true;
      }
    }
    return false;
  }

  isClicked() {
    if (this.clicked_) {
      this.clicked_ = false;
      return true;
    }
    return false;
  }

  static TouchGroup(grp, msg, x, y) {
    for (let e of grp) {
      e.on = false;
    }
    for (let e of grp) {
      if (!e.rect) {
        return false;
      }
      if (XTools.InRect(x, y, ...e.rect)) {
        if (e.type == 1 && msg == 1) {
          e.open = !e.open;
        }
        if (e.type == 2) {
        }
        if (e.type == 0) {
          if (msg == 1) {
            e.cb();
          }
        }
        e.on = true;
        return true;
      }
      if (e.type == 1) {
        if (e.open && RightMenu.TouchGroup(e.group, msg, x, y)) {
          return true;
        }
      }
    }
    return false;
  }
}

module.exports = {
  RightMenu,
};
