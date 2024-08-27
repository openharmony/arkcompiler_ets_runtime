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

const { XTools } = require('../engine/XTools');
const { X2DFast } = require('../engine/graphics/X2DFast');
const { NapiLog } = require('./NapiLog');

class LogParser {
  constructor(result) {
    const regexVT100 = /\x1B\[[0-9;]*[A-Za-z]/g; // 匹配VT100字符的正则表达式
    result = result.replace(regexVT100, '');
    result = result.replace(/\t/g, '    ');
    result = result.replace(/\r/g, '');
    this.logLines_ = result.split('\n');
    this.stat_ = 0;
    this.lineNum_ = 0;
    this.initBlock_ = {
      filePoint: null,
      funcPoint: null,
      blockType: -1,
    };
    this.procBlock_ = {
      blockStat: -1,
      blockCollect: -1,
      oneBlock: -1,
    };
    this.procNormal_ = null;
    this.output_ = {};
  }

  parsing() {
    if (this.lineNum_ >= this.logLines_.length) {
      return false;
    }

    for (let i = 0; i < 300 && this.lineNum_ < this.logLines_.length; i++) {
      this.parseLine(this.logLines_[this.lineNum_]);
      this.lineNum_++;
    }

    XTools.PROC_TO = this.lineNum_ * 20 / this.logLines_.length;
    return true;
  }
  isBlock(l) {
    for (let bt of XTools.CONFIG.BlockTypes) {
      if (l.indexOf(bt) >= 0) {
        this.stat_ = this.initBlock_.blockType;
        this.procBlock_.blockStat = 0;
        this.procBlock_.blockCollect = {
          type: 'block:' + bt,
          func: this.initBlock_.funcPoint,
          file: this.initBlock_.filePoint,
          irList: [],
          startLine: l,
        };
        this.procBlock_.oneBlock = null;
        return true;
      }
    }
    return false;
  }
  splitLast(s) {
    let i = s.lastIndexOf('@');

    return [s.substring(0, i), s.substring(i + 1)];
  }
  isStart(l) {
    //========= After bytecode2circuit lowering [func_main_0@484@arkcompiler/ets_runtime/sd_test/ttabs.abc] ========
    const regexStart = /=+ *After ([a-zA-Z0-9_ ]+) \[([#a-zA-Z0-9_@/.-]+)\] *=+/g;
    //========= After inlining [OthreMath@test@arkcompiler/ets_runtime/sd_test/test.abc] Caller
    //method [func_main_0@641@arkcompiler/ets_runtime/sd_test/test.abc]====================[0m
    const regexStart2 = /=+ *After ([a-zA-Z0-9_ ]+) \[([a-zA-Z0-9_@/.-]+)\] *Caller method \[([#a-zA-Z0-9_@/.]+)\] *=+/g;

    if (l[11] !== '=') {
      return;
    }
    let ret = regexStart.exec(l);
    if (ret) {
      let tt = this.splitLast(ret[2]);
      this.procNormal_ = {
        type: ret[1], //优化类型
        func: tt[0], //函数名
        file: tt[1], //文件名
        irList: [],
        startLine: l,
      };
      this.stat_ = 1;
      [this.initBlock_.funcPoint, this.initBlock_.filePoint] = [tt[0], tt[tt.length - 1]];
      this.initBlock_.blockType = 10;
    }
    else {
      ret = regexStart2.exec(l);
      if (ret) {
        let tt = this.splitLast(ret[2]);
        let tt2 = this.splitLast(ret[3]);
        NapiLog.logInfo(tt[0], 'Caller method(' + this.lineNum_ + '行)', ret[3]);
        this.procNormal_ = {
          type: ret[1] + ' ' + tt[0], //优化类型
          func: tt2[0], //函数名
          file: tt2[1], //文件名
          irList: [],
          startLine: l,
        };
        this.stat_ = 1;
        [this.initBlock_.funcPoint, this.initBlock_.filePoint] = [tt2[0], tt2[tt2.length - 1]];
        this.initBlock_.blockType = 10;
      }
      else {
        if (l.search('After') > 0) {
          alert(l);
        }
      }
    }
  }
  collectNormal(l) {
    let idx = l.search('{"id":');
    if (idx >= 0) {
      let idx2 = l.lastIndexOf('}');
      let str = l.substring(idx, idx2 + 1);

      let ir = JSON.parse(str);
      { //根据XTools.CONFIG.MTypeField切割MType
        let cutPos = [];
        for (let field of XTools.CONFIG.MTypeField) {
          let idx = ir.MType.indexOf(', ' + field);
          if (idx >= 0) {
            cutPos.push(idx);
          }
        }
        cutPos.push(ir.MType.length);
        cutPos.sort((a, b) => { return parseInt(a) - parseInt(b) });
        if (cutPos[0] === 0) {
          cutPos.shift();
        }
        let p = 0;
        let cutResult = [];
        for (let i of cutPos) {
          let tmp = ir.MType.substring(p, i);
          if (tmp.startsWith(', ')) {
            tmp = tmp.substring(2);
          }
          if (tmp.endsWith(', ')) {
            tmp = tmp.substring(0, tmp.length - 2);
          }
          cutResult.push(tmp);
          p = i;
        }
        cutResult.push('inNum=[' + ir.in[0].length + ',' + ir.in[1].length + ',' + ir.in[2].length + ',' + ir.in[3].length + ',' + ir.in[4].length + ']');
        cutResult.push('outNum=' + ir.out.length);
        if ('comment' in ir) {
          cutResult.push('//' + ir.comment);
        }
        ir.maxDetailWidth = 0;
        for (let detail of cutResult) {
          let w = X2DFast.gi().getTextWidth(detail, 14);
          if (ir.maxDetailWidth < w) {
            ir.maxDetailWidth = w;
          }
        }
        ir.detailList = cutResult;
      }
      this.procNormal_.irList.push(ir);
    }
    else {
      //= End typeHitRate: 0.500000 =
      let regexEnd = /=+ End[a-zA-Z:.0-9 ]* =+/g;
      let tt = regexEnd.exec(l);
      if (tt) { //收集结束，入大表l.search('== End ==') > 0
        if (this.procNormal_.irList.length > 0) {
          if (!(this.procNormal_.file in this.output_)) {
            this.output_[this.procNormal_.file] = {};
          }
          if (!(this.procNormal_.func in this.output_[this.procNormal_.file])) {
            this.output_[this.procNormal_.file][this.procNormal_.func] = [];
          }
          this.output_[this.procNormal_.file][this.procNormal_.func].push(this.procNormal_);
        }
        else {
          NapiLog.logError('After和End之间没有指令(' + this.lineNum_ + '行)');
        }
        this.stat_ = 0;
      }
      else {
        NapiLog.logError('After和End之间解析失败(' + (this.lineNum_ + 1) + ')行');
        this.stat_ = 0;
      }
    }
  }
  parseLine(l) {
    switch (this.stat_) {
      case 0: //搜索起始
        if (this.SearchBegin(l) || this.isBlock(l)) {
          return;
        }
        this.isStart(l);
        break;
      case 1: //收集ir表
        this.collectNormal(l);
        break;
      case 10: //收集block一
        if (this.CollectBlock(l)) {
          this.stat_ = 0;
          this.lineNum_ -= 1;
        }
        break;
      case 20: //收集block二
        if (this.CollectBlock2(l)) {
          this.stat_ = 0;
          this.lineNum_ -= 1;
        }
        break;
    }
  }

  static Load(fn, cb) {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', fn);
    xhr.setRequestHeader('Cache-Control', 'no-cache');
    xhr.onreadystatechange = () => {
      if (xhr.readyState === XMLHttpRequest.DONE) {
        if (xhr.status === 200) {
          XTools.PORC_TO = 10;
          cb(fn, xhr.responseText);
        }
      }
    };
    xhr.send();
  }
  NumberStringToArray(ss) {
    let outs = ss.split(',');
    let ret = [];
    for (let s of outs) {
      let ttt = parseInt(s);
      if (!isNaN(ttt)) {
        ret.push(ttt);
      }
    }
    return ret;
  }
  SearchBegin(l) {
    let ret;
    let ib = this.initBlock_;

    if (l.startsWith('[compiler] aot method')) {
      //[compiler] aot method [func_main_0@b.abc] log:
      const regexFuncName = /^\[compiler\] aot method \[([#a-zA-Z0-9_@/.]+)\] (recordName \[[a-zA-Z0-9_]*\] )*log:/g;
      ret = regexFuncName.exec(l);
      if (ret) {
        [ib.funcPoint, ib.filePoint] = this.splitLast(ret[1]);
        ib.blockType = 10;
        return true;
      }
    }
    if (l.startsWith('[compiler] ==================== Before state split')) {
      const regexFuncName2 = /^\[compiler\] =+ Before state split linearizer \[([#a-zA-Z0-9_@/.]+)\] *=*/g;
      ret = regexFuncName2.exec(l);
      if (ret) {
        [ib.funcPoint, ib.filePoint] = this.splitLast(ret[1]);
        ib.blockType = 20;
        return true;
      }
    }
    if (l.startsWith('[compiler] ==================== After graph lineari')) {
      const regexFuncName3 = /^\[compiler\] =+ After graph linearizer \[([#a-zA-Z0-9_@/.]+)\] *=*/g;
      ret = regexFuncName3.exec(l);
      if (ret) {
        [ib.funcPoint, ib.filePoint] = this.splitLast(ret[1]);
        ib.blockType = 20;
        return true;
      }
    }
    return false;
  }
  CollectBlock(l) {
    const regexBlock = [
      /^\[compiler\] B([0-9]+):                               ;preds=([0-9, ]*)$/g, //[compiler] B0:                               ;preds=
      /^\[compiler\]  *Succes:([0-9, ]*)$/g, //[compiler] 	Succes:
      /^\[compiler\]  *Bytecode\[\] = *(Empty)*$/g, //[compiler] 	Bytecode[] = Empty
      /^\[compiler\]  *Trys:([0-9, ]*)$/g, //[compiler] 	Trys:
    ];
    let ret;
    let pb = this.procBlock_;
    if (pb.blockStat === 0) {
      ret = regexBlock[0].exec(l);
      if (ret) {
        pb.oneBlock = {
          id: ret[1],
          op: 'B' + ret[1],
          detailList: [],
          maxDetailWidth: 0,
        };
        pb.oneBlock.in = [[], [], [], [], this.NumberStringToArray(ret[2])];
        return false;
      }
      if (!pb.oneBlock) { //完成了一个block的解析
        if (!(pb.blockCollect.file in this.output_)) {
          this.output_[pb.blockCollect.file] = {};
        }
        if (!(pb.blockCollect.func in this.output_[pb.blockCollect.file])) {
          this.output_[pb.blockCollect.file][pb.blockCollect.func] = [];
        }
        this.output_[pb.blockCollect.file][pb.blockCollect.func].push(pb.blockCollect);
        return true;
      }
      ret = regexBlock[1].exec(l);
      if (ret) {
        pb.oneBlock.out = this.NumberStringToArray(ret[1]);
        return false;
      }
      ret = regexBlock[2].exec(l);
      if (ret) {
        pb.blockStat = 1;
        return false;
      }
    }
    else if (pb.blockStat === 1) { //开始记录bytecode，直到空行，结束这个block
      if (/^\[compiler\] *$/g.test(l)) { //遇到空行，完成block
        if (pb.oneBlock.maxDetailWidth === 0) {
          pb.oneBlock.maxDetailWidth = X2DFast.gi().getTextWidth('Empty', 14);
          pb.oneBlock.detailList.push('Empty');
        }
        pb.blockCollect.irList.push(pb.oneBlock);
        pb.oneBlock = null;
        pb.blockStat = 0;
      }
      else {
        let s = l.substring(11);
        while (s.startsWith(' ')) {
          s = s.substring(1);
        }
        let w = X2DFast.gi().getTextWidth(s, 14);
        if (pb.oneBlock.maxDetailWidth < w) {
          pb.oneBlock.maxDetailWidth = w;
        }
        pb.oneBlock.detailList.push(s);
      }
    }
    return false;
  }
  CollectBlock2(l) {
    const regexBlock = [
      /^\[compiler\] B([0-9]+):/g, //[compiler] B0:                               ;preds=
      /^\[compiler\]  *Preds:([0-9, ]*)$/g,
      /^\[compiler\]  *Succes:([0-9, ]*)$/g, //[compiler] 	Succes:
      /^\[compiler\]  *Bytecode\[\] = *(Empty)*$/g, //[compiler] 	Bytecode[] = Empty
      /^\[compiler\]  *Trys:([0-9, ]*)$/g, //[compiler] 	Trys:
    ];
    let pb = this.procBlock_;
    let ret;
    switch (pb.blockStat) {
      case 0:
        ret = regexBlock[0].exec(l);
        if (ret) {
          pb.oneBlock = {
            id: ret[1],
            op: 'B' + ret[1],
            detailList: [],
            maxDetailWidth: 0,
          };
          pb.blockStat = 1;
          return false;
        }
        if (!pb.oneBlock) { //完成了一个block的解析
          if (!(pb.blockCollect.file in this.output_)) {
            this.output_[pb.blockCollect.file] = {};
          }
          if (!(pb.blockCollect.func in this.output_[pb.blockCollect.file])) {
            this.output_[pb.blockCollect.file][pb.blockCollect.func] = [];
          }
          this.output_[pb.blockCollect.file][pb.blockCollect.func].push(pb.blockCollect);
          return true;
        }
        break;
      case 1:
        ret = regexBlock[1].exec(l);
        if (ret) {
          pb.oneBlock.in = [[], [], [], [], this.NumberStringToArray(ret[1])];
          pb.blockStat = 2;
          return false;
        }
        break;
      case 2:
        ret = regexBlock[2].exec(l);
        if (ret) {
          pb.oneBlock.out = this.NumberStringToArray(ret[1]);
          pb.blockStat = 3;
          return false;
        }
        break;
      case 3: //开始记录bytecode，直到空行，结束这个block
        if (/^\[compiler\] *$/g.test(l)) { //遇到空行，完成block
          if (pb.oneBlock.maxDetailWidth === 0) {
            pb.oneBlock.maxDetailWidth = X2DFast.gi().getTextWidth('Empty', 14);
            pb.oneBlock.detailList.push('Empty');
          }
          pb.blockCollect.irList.push(pb.oneBlock);
          pb.oneBlock = null;
          pb.blockStat = 0;
        }
        else {
          let s = l.substring(11);
          while (s.startsWith(' ')) {
            s = s.substring(1);
          }
          let w = X2DFast.gi().getTextWidth(s, 14);
          if (pb.oneBlock.maxDetailWidth < w) {
            pb.oneBlock.maxDetailWidth = w;
          }
          pb.oneBlock.detailList.push(s);
        }
        return false;
      default:
        return false;
    }
    return false;
  }
}

module.exports = {
  LogParser
};
