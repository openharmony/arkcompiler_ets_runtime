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

const INTYPE = {
  state: 0,
  depend: 1,
  value: 2,
  framestate: 3,
  root: 4,
  other: 5,
};

class NodeTypeMask {
  static NONE = 0;
  static CONTROL = 1 << INTYPE.state;
  static DEPEND = 1 << INTYPE.depend;
  static VALUE = 1 << INTYPE.value;
  static FRAMESTATE = 1 << INTYPE.framestate;
  static ROOT = 1 << INTYPE.root;
  static OTHER = 1 << INTYPE.other;
}

class IrToPicture {
  static INVALID_DEEP = -99999;
  static NODEH = 20;
  static LINE_TYPE = ['state', 'depend', 'value', 'framestate', 'root'];
  static nodeType(ir) {
    if (XTools.CONFIG.OpTypeControl.indexOf(ir.op) >= 0) {
      return 'control';
    }
    if (ir.in[INTYPE.state].length > 0 && XTools.CONFIG.OpNotControl.indexOf(ir.op) === -1) {
      return 'control';
    }
    if (XTools.CONFIG.OpTypeDepend.indexOf(ir.op) >= 0 || ir.in[INTYPE.depend].length > 0) {
      return 'depend';
    }
    if (XTools.CONFIG.OpTypeValue.indexOf(ir.op) >= 0 || ir.in[INTYPE.value].length > 0) {
      return 'value';
    }
    return 'other';
  }
  static nodeTypeMask(ir) {
    let mask = NodeTypeMask.NONE;
    if (XTools.CONFIG.OpTypeControl.indexOf(ir.op) >= 0) {
      mask |= NodeTypeMask.CONTROL;
    }
    if (ir.in[INTYPE.state].length > 0 && XTools.CONFIG.OpNotControl.indexOf(ir.op) === -1) {
      mask |= NodeTypeMask.CONTROL;
    }
    if (XTools.CONFIG.OpTypeDepend.indexOf(ir.op) >= 0 || ir.in[INTYPE.depend].length > 0) {
      mask |= NodeTypeMask.DEPEND;
    }
    if (XTools.CONFIG.OpTypeValue.indexOf(ir.op) >= 0 || ir.in[INTYPE.value].length > 0) {
      mask |= NodeTypeMask.VALUE;
    }
    if (XTools.CONFIG.OpTypeFrameState.indexOf(ir.op) >= 0 || ir.in[INTYPE.framestate].length > 0) {
      mask |= NodeTypeMask.FRAMESTATE;
    }
    if (XTools.CONFIG.OpTypeCircuitRoot.indexOf(ir.op) >= 0 || ir.in[INTYPE.root].length > 0) {
      mask |= NodeTypeMask.ROOT;
    }
    if (mask === NodeTypeMask.NONE) {
      mask = NodeTypeMask.OTHER;
    }
    return mask;
  }
  static isLoopBack(l, nodes) {
    if (XTools.CONFIG.OpTypeLoopBegin.indexOf(nodes[l.toId].ir.op) >= 0 && l.fromId === nodes[l.toId].ir.in[0][1]) {
      return true;
    }
    if (XTools.CONFIG.OpTypeDependSelector.indexOf(nodes[l.toId].ir.op) >= 0 && l.fromId === nodes[l.toId].ir.in[1][1]) {
      return true;
    }
    if (XTools.CONFIG.OpTypeValueSelector.indexOf(nodes[l.toId].ir.op) >= 0 && l.fromId === nodes[l.toId].ir.in[2][1]) {
      return true;
    }
    return false;
  }
  static toPicture(irList, type, isBlock) {
    let nodes = {};
    let entry = -1;
    for (let ir of irList) {//用于生成图的所有节点
      if (type === 0) {//仅控制流
        if (this.nodeType(ir) !== 'control') {
          continue;
        }
      }
      let name;
      if (XTools.CONFIG.OpTypeJsBytecode.indexOf(ir.op) >= 0) {
        name = ir.id + ',' + ir.bytecode;
      }
      else if (ir.typedop) {
        name = ir.id + ',' + ir.typedop;
      }
      else {
        name = ir.id + ',' + ir.op;
      }
      nodes[ir.id] = {
        type: this.nodeType(ir),
        mask: this.nodeTypeMask(ir),
        hide: false,
        inCount: 0,
        in: [],
        inh: {},
        outCount: 0,
        out: [],
        outh: [],
        pos: { x: -1, y: -1 },
        deep: this.INVALID_DEEP,
        name: name,
        nameWidth: X2DFast.gi().getTextWidth(name, 14),
        ir: ir,
      };
      if (entry === -1) {
        entry = ir.id;
      }
    }

    let lines = [];
    let lid = 0;
    this.generateLine(nodes, lines, lid);
    this.resetPicture(nodes, isBlock);

    return {
      nodes: nodes,
      lines: lines,
    };
  }
  static generateLine(nodes, lines, lid) {
    for (let i in nodes) { //生成连接线
      let inId = parseInt(i);
      for (let inP1 = 0; inP1 < nodes[inId].ir.in.length; inP1++) {
        for (let inP2 = 0; inP2 < nodes[inId].ir.in[inP1].length; inP2++) {
          let outId = nodes[inId].ir.in[inP1][inP2];
          if (outId in nodes) {
            let line = {
              lid: lid++,
              lineType: this.LINE_TYPE[inP1],
              inNum: nodes[inId].inCount,
              outNum: nodes[outId].outCount,
              fromId: outId,
              toId: inId,
              inP1: inP1,
              inP2: inP2,
              outP: nodes[outId].ir.out.indexOf(inId),
              used: false,
            };
            nodes[inId].inCount++;
            nodes[inId].in.push(line);
            nodes[outId].outCount++;
            nodes[outId].out.push(line);
            lines.push(line);
          }
        }
      }
    }
  }

  static deepTest(n, nodes, isBlock, stack, dist) {
    try {
      stack.push(n.ir.id);
    }
    catch (e) {
      console.log(1);
    }
    if (stack.length > Object.keys(nodes).length * 2) {
      return true;
    }
    if (stack.length > 1 && n.ir.id === dist) {
      return true;
    }
    for (let i = 0; i < n.out.length; i++) {
      let nout = nodes[n.out[i].toId];
      if (n.deep !== this.INVALID_DEEP) {
        if (nout.deep === this.INVALID_DEEP) {
          nout.deep = n.deep + 1;
          if (this.deepTest(nout, nodes, isBlock, stack, dist)) {
            return true;
          }
        }
        if (nout.deep <= n.deep) {
          if (!this.isLoopBack(n.out[i], nodes) && !isBlock) {
            nout.deep = n.deep + 1;
            if (this.deepTest(nout, nodes, isBlock, stack, dist)) {
              return true;
            }
          }
        }
      }
    }
    stack.pop();
    return false;
  }
  static checkoutLoop(ls) {
    console.log(JSON.stringify(ls));
    let dicts = {};
    for (let l of ls) {
      if (!(l in dicts)) {
        dicts[l] = 1;
      }
      else {
        dicts[l]++;
      }
    }
    console.log(JSON.stringify(dicts, null, 4));
  }
  static TEST_LOOP = true;
  static resetPicture(nodes, isBlock) {
    if (this.TEST_LOOP && Object.keys(nodes).length > 0) {
      for (let k in nodes) {
        if (k === 0) {
          nodes[k].deep = 0;
        }
        else {
          nodes[k].deep = this.INVALID_DEEP;
        }
      }
      let testResult = [];
      this.deepTest(nodes[0], nodes, isBlock, testResult, 0);
      if (testResult.length > 0) {
        this.checkoutLoop(testResult);
      }
    }

    let entry = true;
    let enums = [];
    for (let k in nodes) {
      let n = nodes[k];
      if (n.hide) {
        continue;
      }
      if (entry) {
        n.pos.x = 0;
        n.pos.y = 0;
        n.deep = 0;
        entry = false;
        enums.push(k);
      }
      else {
        n.deep = this.INVALID_DEEP;
      }
    }
    let collectDebug = [];
    while (enums.length > 0) { //12,18,27,28,31,34
      let nextenums = [];
      for (let e = 0; e < enums.length; e++) {
        let k = enums[e];
        let n = nodes[k];
        if (n.hide) {
          continue;
        }
        for (let i = 0; i < n.out.length; i++) {
          let nout = nodes[n.out[i].toId];
          if (n.deep !== this.INVALID_DEEP) {
            if (nout.deep === this.INVALID_DEEP) {
              nout.deep = n.deep + 1;
              nextenums.push(nout.ir.id);
            }
            if (nout.deep <= n.deep) {
              if (!this.isLoopBack(n.out[i], nodes) && !isBlock) {
                nout.deep = n.deep + 1;
                nextenums.push(nout.ir.id);
              }
            }
          }
        }
        for (let i = 0; i < n.in.length; i++) {
          let nin = nodes[n.in[i].fromId];
          if (n.deep !== this.INVALID_DEEP) {
            if (nin.deep === this.INVALID_DEEP) {
              nin.deep = n.deep - 1;
              nextenums.push(nin.ir.id);
            }
            if (nin.deep >= n.deep) {
              if (!this.isLoopBack(n.in[i], nodes) && !isBlock) {
                n.deep = nin.deep + 1;
                nextenums.push(n.ir.id);
              }
            }
          }
        }
      }
      collectDebug.push(enums);

      enums = nextenums;
    }

    let levels = {};
    for (let k in nodes) { //节点分层
      let n = nodes[k];
      if (n.hide) {
        continue;
      }
      if (!(n.deep in levels)) {
        levels[n.deep] = [];
      }
      levels[n.deep].push(n);
    }
    let ty = 50;
    for (let k in nodes) {
      let n = nodes[k];
      let ltypes = [];
      for (let l of n.out) {
        if (ltypes.indexOf(l.lineType) < 0) {
          ltypes.push(l.lineType);
        }
      }
      n.ltypes = ltypes;
      if (n.hide) {
        continue;
      }
      if (n.deep === this.INVALID_DEEP) {
        n.pos.x = this.INVALID_DEEP; //Scr.logicw - 100;
        n.pos.y = ty;
        ty += 50;
      }
    }
    let posy = 0;
    let ks = Object.keys(levels).sort((a, b) => { return parseInt(a) - parseInt(b) });
    for (let k of ks) {
      k = parseInt(k);
      if (k === this.INVALID_DEEP) {
        continue;
      }
      let inCount = 0;
      let outCount = 0;
      let inP = 0;
      for (let i = 0; i < levels[k].length; i++) {
        let n = levels[k];
        if (n.hide) {
          continue;
        }
        for (let j = 0; j < n[i].in.length; j++) {
          let l = n[i].in[j];
          if (!n[i].inh[l.fromId + l.lineType]) {
            n[i].inh[l.fromId + l.lineType] = (inP + 1) * 5;
            inP += 1;
          }
        }
        inCount += Object.keys(n[i].inh).length;

        outCount += n[i].ltypes.length;
      }
      posy += (inCount + 1) * 5;

      let outP = 0;
      for (let i = 0; i < levels[k].length; i++) {
        let n = levels[k];
        if (n.hide) {
          continue;
        }
        for (let j = 0; j < n[i].out.length; j++) {
          n[i].outh[j] = (outP + 1 + n[i].ltypes.indexOf(n[i].out[j].lineType)) * 5;
        }
        n[i].pos.y = posy;
        outP += n[i].ltypes.length;
      }

      posy += (outCount + 1) * 5 + this.NODEH;

      let w = 0;
      for (let i = 0; i < levels[k].length; i++) { //当前行总宽度
        w += levels[k][i].nameWidth + 20;
      }
      let x = -w / 2;
      for (let i = 0; i < levels[k].length; i++) { //每个节点x偏移
        levels[k][i].pos.x = x + 10;
        x += levels[k][i].nameWidth + 20;
      }
    }
  }
}

module.exports = {
  IrToPicture
};