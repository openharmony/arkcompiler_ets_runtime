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

export class XMat4 {
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

let tmpmat = new XMat4();
