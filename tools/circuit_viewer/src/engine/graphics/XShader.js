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

const { shaderFastVs, shaderFastFs } = require('./shaders/shader_fast');
const { NapiLog } = require('../../ir/NapiLog.js');
import { gl } from '../GLFrame.js';

export class XShader {
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

    this.shaderFast.position = gl.getAttribLocation(
      this.shaderFast['program'],
      'position'
    );
    this.shaderFast.aTexCoord = gl.getAttribLocation(
      this.shaderFast['program'],
      'aTexCoord'
    );
    this.shaderFast.ext1 = gl.getAttribLocation(
      this.shaderFast['program'],
      'ext1'
    );
    this.shaderFast.ext2 = gl.getAttribLocation(
      this.shaderFast['program'],
      'ext2'
    );
    this.shaderFast.inColor = gl.getAttribLocation(
      this.shaderFast['program'],
      'inColor'
    );

    this.shaderFast.uMat = gl.getUniformLocation(
      this.shaderFast['program'],
      'uMat'
    );

    this.shaderFast.tex = {};
    for (let i = 0; i < 16; i++) {
      this.shaderFast.tex[i] = gl.getUniformLocation(
        this.shaderFast['program'],
        'tex' + i
      );
    }

    this.shaders[XShader.ID_SHADER_FAST] = this.shaderFast;
    this.use(XShader.ID_SHADER_FAST);
  }
  use(sid) {
    gl.useProgram(this.shaders[sid]['program']);
    this.pUseingShader = this.shaders[sid];
    return this.pUseingShader;
  }
  initShader(vss, fss) {
    let vs = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vs, vss);
    gl.compileShader(vs);
    if (!gl.getShaderParameter(vs, gl.COMPILE_STATUS)) {
      NapiLog.logError(
        'error occured compiling the shaders:' + gl.getShaderInfoLog(vs)
      );
      return null;
    }

    let fs = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fs, fss);
    gl.compileShader(fs);
    if (!gl.getShaderParameter(fs, gl.COMPILE_STATUS)) {
      NapiLog.logError(
        'error occured compiling the shaders:' + gl.getShaderInfoLog(fs)
      );
      return null;
    }

    let ret = gl.createProgram();

    gl.attachShader(ret, vs);
    gl.attachShader(ret, fs);
    gl.linkProgram(ret);

    if (!gl.getProgramParameter(ret, gl.LINK_STATUS)) {
      NapiLog.logError('unable to initialize!');
      return;
    }

    gl.deleteShader(vs);
    gl.deleteShader(fs);
    return ret;
  }
}

XShader.ID_SHADER_FAST = 2;

XShader.pinstance_ = null;
