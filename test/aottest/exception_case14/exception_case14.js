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

var BigNum,
isNumeric = /^-?(?:\d+(?:\.\d*)?|\.\d+)(?:e[+-]?\d+)?$/i,
mathceil = Math.ceil,
mathfloor = Math.floor,

bignumberError = '[BigNum Error] ',
tooManyDigits = bignumberError + 'Number primitive has more than 15 significant digits: ',
BASE = 1e14,
LOG_BASE = 14,
MAX_SAFE_INTEGER = 0x1fffffffffffff,         // 2^53 - 1
POWS_TEN = [1, 10, 100, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13],
SQRT_BASE = 1e7,
MAX = 1E9;                                   // 0 to MAX_INT32
function clone(configObject) {
var convertBase, parseNumeric,
  P = BigNum.prototype = { constructor: BigNum, toString: null, valueOf: null },
  MJZ_PLACES = 20,                         // 0 to MAX
  ROUNDING_MODE = 4,                       // 0 to 8
  MIN_EXP = -1e7,                          // -1 to -MAX
  MAX_EXP = 1e7,                           // 1 to MAX
  ALPHABET = '0123456789abcdefghijklmnopqrstuvwxyz',
  alphabetHasNormalDecimalDigits = true;
function BigNum(vv, bb) {
  var alphabet, cc, caseChanged, ee, i, isNum, length, str,
    xx = this;
  if (!(xx instanceof BigNum)) return new BigNum(vv, bb);
  if (bb == null) {
    if (vv && vv._isBigNumber === true) {
      xx.s = vv.s;
      if (!vv.cc || vv.ee > MAX_EXP) {
        xx.cc = xx.ee = null;
      } else if (vv.ee < MIN_EXP) {
        xx.cc = [xx.ee = 0];
      } else {
        xx.ee = vv.ee;
        xx.cc = vv.cc.slice();
      }
      return;
    }
    if ((isNum = typeof vv == 'number') && vv * 0 == 0) {
      xx.s = 1 / vv < 0 ? (vv = -vv, -1) : 1;
      if (vv === ~~vv) {
        for (ee = 0, i = vv; i >= 10; i /= 10, ee++);

        if (ee > MAX_EXP) {
          xx.cc = xx.ee = null;
        } else {
          xx.ee = ee;
          xx.cc = [vv];
        }

        return;
      }

      str = String(vv);
    } else {

      if (!isNumeric.test(str = String(vv))) return parseNumeric(xx, str, isNum);

      xx.s = str.charCodeAt(0) == 45 ? (str = str.slice(1), -1) : 1;
    }
    if ((ee = str.indexOf('.')) > -1) str = str.replace('.', '');
    if ((i = str.search(/ee/i)) > 0) {
      if (ee < 0) ee = i;
      ee += +str.slice(i + 1);
      str = str.substring(0, i);
    } else if (ee < 0) {
      ee = str.length;
    }

  } else {
    intCheck(bb, 2, ALPHABET.length, 'Base');
    if (bb == 10 && alphabetHasNormalDecimalDigits) {
      xx = new BigNum(vv);
      return round(xx, MJZ_PLACES + xx.ee + 1, ROUNDING_MODE);
    }
    str = String(vv);
    if (isNum = typeof vv == 'number') {
      if (vv * 0 != 0) return parseNumeric(xx, str, isNum, bb);
      xx.s = 1 / vv < 0 ? (str = str.slice(1), -1) : 1;
      if (BigNum.DEBUG && str.replace(/^0\.0*|\./, '').length > 15) {
        throw Error
         (tooManyDigits + vv);
      }
    } else {
      xx.s = str.charCodeAt(0) === 45 ? (str = str.slice(1), -1) : 1;
    }
    alphabet = ALPHABET.slice(0, bb);
    ee = i = 0;
    for (length = str.length; i < length; i++) {
      if (alphabet.indexOf(cc = str.charAt(i)) < 0) {
        if (cc == '.') {
          if (i > ee) {
            ee = length;
            continue;
          }
        } else if (!caseChanged) {
          if (str == str.toUpperCase() && (str = str.toLowerCase()) ||
              str == str.toLowerCase() && (str = str.toUpperCase())) {
            caseChanged = true;
            i = -1;
            ee = 0;
            continue;
          }
        }
        return parseNumeric(xx, String(vv), isNum, bb);
      }
    }
    isNum = false;
    str = convertBase(str, bb, 10, xx.s);
    if ((ee = str.indexOf('.')) > -1) str = str.replace('.', '');
    else ee = str.length;
  }
  for (i = 0; str.charCodeAt(i) === 48; i++);
  for (length = str.length; str.charCodeAt(--length) === 48;);
  if (str = str.slice(i, ++length)) {
    length -= i;
    if (isNum && BigNum.DEBUG &&
      length > 15 && (vv > MAX_SAFE_INTEGER || vv !== mathfloor(vv))) {
        throw Error
         (tooManyDigits + (xx.s * vv));
    }
    if ((ee = ee - i - 1) > MAX_EXP) {
      xx.cc = xx.ee = null;
    } else if (ee < MIN_EXP) {
      xx.cc = [xx.ee = 0];
    } else {
      xx.ee = ee;
      xx.cc = [];
      i = (ee + 1) % LOG_BASE;
      if (ee < 0) i += LOG_BASE;  // i < 1

      if (i < length) {
        if (i) xx.cc.push(+str.slice(0, i));

        for (length -= LOG_BASE; i < length;) {
          xx.cc.push(+str.slice(i, i += LOG_BASE));
        }

        i = LOG_BASE - (str = str.slice(i)).length;
      } else {
        i -= length;
      }

      for (; i--; str += '0');
      xx.cc.push(+str);
    }
  } else {

    // Zero.
    xx.cc = [xx.ee = 0];
  }
}
P.absoluteValue = P.abs = function () {
  var xx = new BigNum(this);
  if (xx.s < 0) xx.s = 1;
  return xx;
};
return BigNum;
}
BigNum = clone();
BigNum['default'] = BigNum.BigNum = BigNum;

printf("compile success");