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

const { Scr } = require('../engine/XDefine');
const { XTools } = require('../engine/XTools');
const { XButton } = require('../engine/control/XButton');
const { XScroll } = require('../engine/control/XScroll');
const { XSelect } = require('../engine/control/XSelect');
const { X2DFast } = require('../engine/graphics/X2DFast');
const { XTexture } = require('../engine/graphics/XTexture');
const { CanvasInput } = require('./CanvasInput');
const { IrToPicture } = require('./IrToPicture');
const { LogParser } = require('./LogParser');
const { NapiLog } = require('./NapiLog');

const INTYPE_STR = ['state', 'depend', 'value', 'framestate', 'root', 'other'];

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
    this.selectFunc_ = new XSelect([], '');
    this.selectFunc_.move(tx, ty, 290, 20);
    this.selectFunc_.registCallback(this.changeFunc.bind(this));

    tx += 290 + 10;
    this.selectMethod_ = new XSelect([], '');
    this.selectMethod_.move(tx, ty, 250, 20);
    this.selectMethod_.registCallback(this.changeMethod.bind(this));

    tx = 10;
    ty += 30;
    this.btnGo_ = [];
    this.mask_ = 0xffff;
    for (let i = 0; i < INTYPE_STR.length; i++) {
      let bname = INTYPE_STR[i] + '✔️'; //❌
      let bw = X2DFast.gi().getTextWidth(bname, 14) + 6;
      let btn = new XButton(tx, ty, bw, 20, bname);
      btn.inTypeId_ = i;
      btn.inTypeMask_ = 1;
      btn.onClicked_ = () => {
        btn.inTypeMask_ = 1 - btn.inTypeMask_;
        btn.name_ = INTYPE_STR[btn.inTypeId_] + (btn.inTypeMask_ === 1 ? '✔️' : '❌');
        this.mask_ = (this.mask_ & ~(1 << btn.inTypeId_)) | (btn.inTypeMask_ << btn.inTypeId_);
        this.changeVisable();
      };
      this.btnGo_.push(btn);
      tx += bw + 10;
    }

    tx = 10;
    ty += 30;
    let bms = [['隐藏选中(Hide selected)', () => { this.hideNode(0); }],
    ['隐藏未选中(Hide unselected)', () => { this.hideNode(1); }],
    ['显示隐藏(Show hide)', () => { this.hideNode(2); }],
    ['选中前继(Select pred)', () => { this.selectNode(0); }],
    ['选中后继(Select suc)', () => { this.selectNode(1); }],
    ['刷新(refresh)', () => { this.freshNode(); }]];
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
    });

    this.scrollY_ = new XScroll({ type: 'right' });
    this.scrollX_ = new XScroll({ type: 'button' });
    this.scrollY_.move(Scr.logicw - 20, 100, 20, Scr.logich - 100 - 20);
    this.scrollX_.move(20, Scr.logich - 20, Scr.logicw - 40, 20);

    this.hideNodeIds_ = [];
    this.pointFile_ = files[0];
  }
  freshNode() {
    this.scrollY_.setBarOff(0);
    this.scrollX_.setBarOff(0.5);
    IrToPicture.resetPicture(this.visable_.nodes, this.direct_.type.startsWith('block:'));
  }
  hideNode(type) {
    if (type === 0) { //隐藏选中
      this.hideNodeIds_ = this.hideNodeIds_.concat(this.selectPoint_);
    }
    else if (type === 1) { //隐藏未选中
      let nodes = this.visable_.nodes;
      for (let k in nodes) {
        if (this.selectPoint_.indexOf(parseInt(nodes[k].ir.id)) >= 0) {
          continue;
        }
        this.hideNodeIds_.push(parseInt(nodes[k].ir.id));
      }
    }
    else { //显示所有
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
      if (type === 0 && (n1.mask & this.mask_) !== 0 && this.selectPoint_.indexOf(id2) >= 0) { //选中前继
        idx = this.hideNodeIds_.indexOf(id1);
        sel.add(id1);
      }
      if (type === 1 && (n2.mask & this.mask_) !== 0 && this.selectPoint_.indexOf(id1) >= 0) { //选中后继
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

          method.irAll = IrToPicture.toPicture(method.irList, 1, method.type.startsWith('block:'));
          method.loaded = true;
        }
      }
    }
    if (loadonce === 0) {
      XTools.PROC_TO = 20 + procto / total * 80;
      return true;
    }
    XTools.PROC_TO = 100;
    this.loaded_ = true;
    this.changeFile(this.pointFile_);
    NapiLog.logInfo('load cost', Date.now() - this.t1_);
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
      methods.push((i + 1) + ',' + this.data_[this.pointFile_][this.pointFunc_][i].type);
    }
    this.selectMethod_.resetList(methods, methods[0]);
    this.changeMethod(methods[0]);
  }
  changeMethod(name) {
    this.pointMethod_ = name;
    let p = parseInt(name.split(',')[0]) - 1;
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
        n.hide = (n.mask & this.mask_) === 0;
        if (!n.hide) {
          showNodes.push(k);
        }
      }
    }
    for (let k of showNodes) {
      let n = nodes[k];
      for (let i = 0; i < 5; i++) {
        if ((this.mask_ & (1 << i) !== 0) && (n.mask & (1 << i) !== 0)) { //进入点也加进来
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
      if (n.deep !== IrToPicture.INVALID_DEEP) {
        levely.add(n.pos.y);
      }
    }
    return Array.from(levely).sort((a, b) => { return parseInt(a) - parseInt(b) });
  }
  drawSmallMap(nodes, x1, x2, y1, y2) {
    if (x1 === x2 || y2 === y1) {
      return;
    }
    let [tx, ty, w, h] = this.smallMapRect;
    X2DFast.gi().fillRect(tx, ty, w, h, 0x80000000);

    let sw = w / (x2 - x1);
    let sh = h / (y2 - y1);

    let dh = Math.max(20 * sh, 1);
    for (let k in nodes) { //画节点
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
      X2DFast.gi().drawText('Loading ' + XTools.PROC_TO.toFixed(1) + '%', 20, Scr.logicw / 2, Scr.logich / 2, 1, 1, 0, -2, -2, 0xff000000);
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
      if (n.deep !== IrToPicture.INVALID_DEEP) {
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
    let mmx1 = this.drawLines(this.offx_, this.offy_, nodes, lines, levely, [minx - 20, maxx + 20], false); //未选中的线
    for (let k in nodes) { //画节点
      let n = nodes[k];
      if (n.deep === IrToPicture.INVALID_DEEP) {
        if (n.pos.x === IrToPicture.INVALID_DEEP) {
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
    this.drawLines(this.offx_, this.offy_, nodes, lines, levely, [minx - 20, maxx + 20], true); //选中的线
    for (let ln of this.selectLines_) {
      let [r, g, b, a] = XTexture.ExpandColor(ln[4]);
      r = Math.max(0, r * 255 - 32);
      g = Math.max(0, g * 255 - 32);
      b = Math.max(0, b * 255 - 32);
      this.drawLine(ln[0], ln[1], ln[2], ln[3], 0xff000000 | (r << 16) | (g << 8) | b, ln[5] + 1);
    }
    if (mouseOn >= 0) {
      let n = nodes[mouseOn]; //显示选中节点的信息
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
      if (n.pos.x < x1) {
        x1 = n.pos.x;
      }
      if (n.pos.x + n.nameWidth > x2) {
        x2 = n.pos.x + n.nameWidth;
      }

      if (n.pos.y < y1) {
        y1 = n.pos.y;
      }
      if (n.pos.y + n.nameWidth > y2) {
        y2 = n.pos.y + IrToPicture.NODEH;
      }
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
    };
    let scrollW = x2 - x1;
    let scrollH = y2 - y1;
    this.dragScoll.hh = scrollH - Scr.logich;
    this.dragScoll.ww = scrollW - Scr.logicw;
    if (this.dragScoll.hh < 1) {
      this.dragScoll.hh = 1;
    }
    if (this.dragScoll.ww < 1) {
      this.dragScoll.ww = 1;
    }
    if (this.drapBackground_) {
      this.scrollY_.setBarOff(-(this.offy_ + this.dragScoll.y1) / this.dragScoll.hh);
      this.scrollX_.setBarOff(-(this.offx_ + this.dragScoll.x1) / this.dragScoll.ww);
    }
    else {
      this.offy_ = (-this.scrollY_.getBarOff()) * this.dragScoll.hh - this.dragScoll.y1;
      this.offx_ = (-this.scrollX_.getBarOff()) * this.dragScoll.ww - this.dragScoll.x1;
    }
    if (this.dragScoll.hh > 1) {
      this.scrollY_.move(Scr.logicw - 20, 100, 20, Scr.logich - 100 - 20).draw();
    }
    if (this.dragScoll.ww > 1) {
      this.scrollX_.move(20, Scr.logich - 20, Scr.logicw - 40, 20).draw();
    }

    this.drawSmallMap(nodes, x1, x2, y1, y2);

    if (this.searchInput) {
      let x = this.searchInput.pos[0];
      let y = this.searchInput.pos[1];
      let w = this.searchInput.pos[2];
      let h = this.searchInput.pos[3];
      X2DFast.gi().fillRect(x, y, w, h, 0x80000000);

      let searchResultTxt =
        this.searchInput.result.length === 0 ? '0/0' :
          this.searchInput.point + 1 + '/' + this.searchInput.result.length;

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
    return i1 + 1 === i2;
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
            mmx[lor] += lor === 0 ? aaa : -aaa;
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

      if (select !== selected) {
        if (this.checkLevel(levely, n1, n2)) { }
        else {
          mmx[lor] += lor === 0 ? -aaa : aaa;
        }
        continue;
      }

      let c = 0xffc0c0c0;
      let lw = 1;

      if (selected) { //选中的点进出的线使用指定的颜色，增加线宽
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
        let lx = n1.outhx[n1.outh[l.outNum]]; //n1.outhx[l.outNum] 或 mmx[lor]
        let ly = n2.inh[l.fromId + l.lineType]; //n2.inh[l.inNum] 或 n2.inh[n1.ir.id]

        ls.push([x1, y1, x1, y1 + n1.outh[l.outNum], c, lw]);
        ls.push([x1, y1 + n1.outh[l.outNum], lx, y1 + n1.outh[l.outNum], c, lw]);
        ls.push([lx, y1 + n1.outh[l.outNum], lx, y2 - ly, c, lw]);
        ls.push([lx, y2 - ly, x2, y2 - ly, c, lw]);
        ls.push([x2, y2 - ly, x2, y2, c, lw]);
        mmx[lor] += lor === 0 ? -aaa : aaa;
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
    if (x1 === x2) {
      if (y1 > y2) {
        [y1, y2] = [y2, y1];
      }
      X2DFast.px2f.fillRect(x1, y1, lw, y2 - y1 + lw, c);
      if (XTools.InRect(XTools.MOUSE_POS.x, XTools.MOUSE_POS.y, x1 - 1, y1, lw + 2, y2 - y1)) {
        return true;
      }
    }
    else if (y1 === y2) {
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
    if (x1 === x2 || y1 === y2) {
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

  checkMsgAndDrapSelect_(msg, x, y){
    if (msg === 3 && this.drapSelect_) {
      let nodes = this.visable_.nodes;
      for (let k of this.selectPoint_) {
        nodes[k].pos.x += this.drapSelect_.dx;
        nodes[k].pos.y += this.drapSelect_.dy;
      }
      this.drapSelect_ = null;
    }
  }

  checkDrapBackground_(msg, x, y){
    if (this.drapBackground_) {
      if (msg === 2) {
        this.offx_ -= this.drapBackground_.x - x;
        this.offy_ -= this.drapBackground_.y - y;
        this.drapBackground_.x = x;
        this.drapBackground_.y = y;
      }
      return true;
    }
  }

  checkDrapSelect_(msg, x, y){
    if (this.drapSelect_) {
      if (msg === 2) {
        if (Math.abs(this.drapSelect_.x - x) > 10 ||
          Math.abs(this.drapSelect_.y - y) > 10 ||
          this.drapSelect_.dx !== 0 ||
          this.drapSelect_.dy !== 0) {
          this.drapSelect_.dx -= this.drapSelect_.x - x;
          this.drapSelect_.dy -= this.drapSelect_.y - y;
          this.drapSelect_.x = x;
          this.drapSelect_.y = y;
        }
      }
      return true;
    }
  }

  checkSearchInput(msg, x, y){
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
  }

  checkMsg(msg, x, y){
    if (msg === 1) {
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
          };
          return true;
        }
      }
      this.selectPoint_ = [];
    }
  }

  onTouch(msg, x, y) {
    if (this.loading()) {
      return true;
    }
    if (this.smallMapLocked_) {
      if (msg === 2) {
        this.resetOffset(x, y);
      }
      if (msg === 3) {
        this.smallMapLocked_ = false;
      }
      return true;
    }
    if (msg === 6) {
      this.drapBackground_ = null;
    }
    this.checkMsgAndDrapSelect_(msg, x, y)
    this.checkDrapBackground_(msg, x, y)
    this.checkDrapSelect_(msg, x, y)
    if (this.scrollX_.onTouch(msg, x, y)) {
      return true;
    }
    if (this.scrollY_.onTouch(msg, x, y)) {
      return true;
    }
    if (XTools.InRect(x, y, ...this.smallMapRect)) {
      if (msg === 1) {
        this.resetOffset(x, y);
        this.smallMapLocked_ = true;
      }
      return true;
    }
    this.checkSearchInput(msg, x, y)
    for (let i = this.btnGo_.length - 1; i >= 0; i--) {
      if (this.btnGo_[i].onTouch(msg, x, y)) {
        return true;
      }
    }
    this.checkMsg(msg, x, y)
    if (msg === 4) {
      this.drapBackground_ = {
        x: x,
        y: y,
      };
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
      case 'PageUp':
        this.selectNode(0);
        return true;
      case 'PageDown':
        this.selectNode(1);
        return true;
      case 'H':
      case 'h':
        this.hideNode(0);
        return true;
      case ' ':
        this.hideNode(1);
        return true;
      case 'S':
      case 's':
        this.hideNode(2);
        return true;
      case 'Enter':
        this.freshNode();
        return true;
    }
    if (k === 'ctrl+f' || k === 'ctrl+F') {
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
              let searchName;
              if (XTools.CONFIG.OpTypeJsBytecode.indexOf(n.ir.op) >= 0) {
                searchName = n.ir.bytecode;
              }
              else if (n.ir.typedop) {
                searchName = n.ir.typedop;
              }
              else {
                searchName = n.ir.op;
              }
              if (n.ir.id === v || searchName.indexOf(v) >= 0 || (isRegExp(v) && searchName.match(v))) {
                this.searchInput.result.push(i);
              }
            }
            if (this.searchInput.result.length > 0) {
              this.locateNode(this.searchInput.result[0]);
              this.searchInput.point = 0;
            }
          }
        }, this.findNext.bind(this));
      };
      CanvasInput.SetSafeArea(...this.searchInput.pos);
      this.searchInput.Open();
    }
    return false;
  }
}

module.exports = {
  IrViewer
};