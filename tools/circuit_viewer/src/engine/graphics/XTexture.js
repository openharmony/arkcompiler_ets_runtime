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

import { gl } from '../GLFrame.js';

export class XTexture {
  static gi() {
    if (XTexture.pinstance_ === null) XTexture.pinstance_ = new XTexture();
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
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, tex);
    gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  }
  loadTextureFromImage(path, keepdata = false) {
    if (path === 'CUSTOM_TEXTURE_1') {
      var rid = this.ximages.length;

      var texture = gl.createTexture();
      XTexture.initTextureStatus(texture);

      let tmp = new Uint8Array([255, 255, 255, 255]);
      gl.texImage2D(
        gl.TEXTURE_2D,
        0,
        gl.RGBA,
        1,
        1,
        0,
        gl.RGBA,
        gl.UNSIGNED_BYTE,
        tmp
      );

      this.ximages[rid] = { stat: 1, path: path, tex: texture, w: 1, h: 1 };
      return rid;
    } else {
      for (let i = 0; i < this.ximages.length; i++) {
        if (this.ximages[i]['path'] === path) {
          return i;
        }
      }
      var rid = this.ximages.length;
      this.ximages[rid] = { stat: 0, path: path, tex: null };
      var image = new Image();
      image.src = path; //"http://localhost:8910/"+
      image.onload = function () {
        var texture = gl.createTexture();
        XTexture.initTextureStatus(texture);

        gl.texImage2D(
          gl.TEXTURE_2D,
          0,
          gl.RGBA,
          gl.RGBA,
          gl.UNSIGNED_BYTE,
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
    if (this.ximages[rid].stat !== 1) return -1;

    if (w === -1) w = ww;
    if (h === -1) h = hh;
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
    if (ww === -1) ww = this.ximages[rid].w;
    if (hh === -1) hh = this.ximages[rid].h;
    if (w ===-1) w = ww;
    if (h === -1) h = hh;
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

    var texture = gl.createTexture();
    XTexture.initTextureStatus(texture);
    gl.texImage2D(
      gl.TEXTURE_2D,
      0,
      gl.RGBA,
      width,
      height,
      0,
      gl.RGBA,
      gl.UNSIGNED_BYTE,
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
    while (x === 256) {
      h -= 1;
      for (x = 0; x < 128; x++) {
        let p = (h * 1024 + x) * 4;
        if (imgd[p] !== 0) break;
      }
    }
    let y = h;
    while (y === h) {
      w -= 1;
      for (y = 0; y < h; y++) {
        let p = (y * 1024 + w) * 4;
        if (imgd[p] !== 0) break;
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
        if ((this.textImgs[k].mask & (mask << i)) === 0) {
          off = i;
          break;
        }
      }
      if (off !== -1) {
        rid = k;
        break;
      }
    }
    if (rid === -1) {
      rid = this.loadTexture(1024, 1024);
      this.textImgs[rid] = { mask: 0 };
      off = 0;
    }
    let cid = this.makeCut(rid, 0, off * 32, w, h);
    this.textImgs[rid]['mask'] |= mask << off;
    this.textIdxs[textIdx] = { cid: cid, rid: rid, mask: mask << off, time: this.timenow(), };
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, this.ximages[rid].tex);
    gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, off * 32, gl.RGBA, gl.UNSIGNED_BYTE, imgd);
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
      ((c >> 24) & 0xff) / 255]; //r,g,b,a
  }
}
XTexture.pinstance_ = null;
