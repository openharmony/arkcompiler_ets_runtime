/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/controlFlowCaching.ts ===
declare function AssertType(value:any, type:string):void;
// Repro for #8401

function f(dim, offsets, arr, acommon, centerAnchorLimit, g, has, lin) {
    let isRtl = this._isRtl();  // chart mirroring
AssertType(isRtl, "any");
AssertType(this._isRtl(), "any");
AssertType(this._isRtl, "any");
AssertType(this, "any");

    // prepare letiable
    let o = this.opt, ta = this.chart.theme.axis, position = o.position,
AssertType(o, "any");
AssertType(this.opt, "any");
AssertType(this, "any");
AssertType(ta, "any");
AssertType(this.chart.theme.axis, "any");
AssertType(this.chart.theme, "any");
AssertType(this.chart, "any");
AssertType(this, "any");
AssertType(position, "any");
AssertType(o.position, "any");

        leftBottom = position !== "rightOrTop", rotation = o.rotation % 360,
AssertType(leftBottom, "boolean");
AssertType(position !== "rightOrTop", "boolean");
AssertType(position, "any");
AssertType("rightOrTop", "string");
AssertType(rotation, "number");
AssertType(o.rotation % 360, "number");
AssertType(o.rotation, "any");
AssertType(360, "int");

        start, stop, titlePos, titleRotation = 0, titleOffset, axisVector, tickVector, anchorOffset, labelOffset, labelAlign,
AssertType(start, "any");
AssertType(stop, "any");
AssertType(titlePos, "any");
AssertType(titleRotation, "number");
AssertType(0, "int");
AssertType(titleOffset, "any");
AssertType(axisVector, "any");
AssertType(tickVector, "any");
AssertType(anchorOffset, "any");
AssertType(labelOffset, "any");
AssertType(labelAlign, "any");

        labelGap = this.chart.theme.axis.tick.labelGap,
AssertType(labelGap, "any");
AssertType(this.chart.theme.axis.tick.labelGap, "any");
AssertType(this.chart.theme.axis.tick, "any");
AssertType(this.chart.theme.axis, "any");
AssertType(this.chart.theme, "any");
AssertType(this.chart, "any");
AssertType(this, "any");

        taFont = o.font || (ta.majorTick && ta.majorTick.font) || (ta.tick && ta.tick.font),
AssertType(taFont, "any");
AssertType(o.font || (ta.majorTick && ta.majorTick.font) || (ta.tick && ta.tick.font), "any");
AssertType(o.font || (ta.majorTick && ta.majorTick.font), "any");
AssertType(o.font, "any");
AssertType((ta.majorTick && ta.majorTick.font), "any");
AssertType(ta.majorTick && ta.majorTick.font, "any");
AssertType(ta.majorTick, "any");
AssertType(ta.majorTick.font, "any");
AssertType(ta.majorTick, "any");
AssertType((ta.tick && ta.tick.font), "any");
AssertType(ta.tick && ta.tick.font, "any");
AssertType(ta.tick, "any");
AssertType(ta.tick.font, "any");
AssertType(ta.tick, "any");

        taTitleFont = o.titleFont || (ta.title && ta.title.font),
AssertType(taTitleFont, "any");
AssertType(o.titleFont || (ta.title && ta.title.font), "any");
AssertType(o.titleFont, "any");
AssertType((ta.title && ta.title.font), "any");
AssertType(ta.title && ta.title.font, "any");
AssertType(ta.title, "any");
AssertType(ta.title.font, "any");
AssertType(ta.title, "any");

        taFontColor = o.fontColor || (ta.majorTick && ta.majorTick.fontColor) || (ta.tick && ta.tick.fontColor) || "black",
AssertType(taFontColor, "any");
AssertType(o.fontColor || (ta.majorTick && ta.majorTick.fontColor) || (ta.tick && ta.tick.fontColor) || "black", "any");
AssertType(o.fontColor || (ta.majorTick && ta.majorTick.fontColor) || (ta.tick && ta.tick.fontColor), "any");
AssertType(o.fontColor || (ta.majorTick && ta.majorTick.fontColor), "any");
AssertType(o.fontColor, "any");
AssertType((ta.majorTick && ta.majorTick.fontColor), "any");
AssertType(ta.majorTick && ta.majorTick.fontColor, "any");
AssertType(ta.majorTick, "any");
AssertType(ta.majorTick.fontColor, "any");
AssertType(ta.majorTick, "any");
AssertType((ta.tick && ta.tick.fontColor), "any");
AssertType(ta.tick && ta.tick.fontColor, "any");
AssertType(ta.tick, "any");
AssertType(ta.tick.fontColor, "any");
AssertType(ta.tick, "any");
AssertType("black", "string");

        taTitleFontColor = o.titleFontColor || (ta.title && ta.title.fontColor) || "black",
AssertType(taTitleFontColor, "any");
AssertType(o.titleFontColor || (ta.title && ta.title.fontColor) || "black", "any");
AssertType(o.titleFontColor || (ta.title && ta.title.fontColor), "any");
AssertType(o.titleFontColor, "any");
AssertType((ta.title && ta.title.fontColor), "any");
AssertType(ta.title && ta.title.fontColor, "any");
AssertType(ta.title, "any");
AssertType(ta.title.fontColor, "any");
AssertType(ta.title, "any");
AssertType("black", "string");

        taTitleGap = (o.titleGap == 0) ? 0 : o.titleGap || (ta.title && ta.title.gap) || 15,
AssertType(taTitleGap, "any");
AssertType((o.titleGap == 0) ? 0 : o.titleGap || (ta.title && ta.title.gap) || 15, "any");
AssertType((o.titleGap == 0), "boolean");
AssertType(o.titleGap == 0, "boolean");
AssertType(o.titleGap, "any");
AssertType(0, "int");
AssertType(0, "int");
AssertType(o.titleGap || (ta.title && ta.title.gap) || 15, "any");
AssertType(o.titleGap || (ta.title && ta.title.gap), "any");
AssertType(o.titleGap, "any");
AssertType((ta.title && ta.title.gap), "any");
AssertType(ta.title && ta.title.gap, "any");
AssertType(ta.title, "any");
AssertType(ta.title.gap, "any");
AssertType(ta.title, "any");
AssertType(15, "int");

        taTitleOrientation = o.titleOrientation || (ta.title && ta.title.orientation) || "axis",
AssertType(taTitleOrientation, "any");
AssertType(o.titleOrientation || (ta.title && ta.title.orientation) || "axis", "any");
AssertType(o.titleOrientation || (ta.title && ta.title.orientation), "any");
AssertType(o.titleOrientation, "any");
AssertType((ta.title && ta.title.orientation), "any");
AssertType(ta.title && ta.title.orientation, "any");
AssertType(ta.title, "any");
AssertType(ta.title.orientation, "any");
AssertType(ta.title, "any");
AssertType("axis", "string");

        taMajorTick = this.chart.theme.getTick("major", o),
AssertType(taMajorTick, "any");
AssertType(this.chart.theme.getTick("major", o), "any");
AssertType(this.chart.theme.getTick, "any");
AssertType(this.chart.theme, "any");
AssertType(this.chart, "any");
AssertType(this, "any");
AssertType("major", "string");
AssertType(o, "any");

        taMinorTick = this.chart.theme.getTick("minor", o),
AssertType(taMinorTick, "any");
AssertType(this.chart.theme.getTick("minor", o), "any");
AssertType(this.chart.theme.getTick, "any");
AssertType(this.chart.theme, "any");
AssertType(this.chart, "any");
AssertType(this, "any");
AssertType("minor", "string");
AssertType(o, "any");

        taMicroTick = this.chart.theme.getTick("micro", o),
AssertType(taMicroTick, "any");
AssertType(this.chart.theme.getTick("micro", o), "any");
AssertType(this.chart.theme.getTick, "any");
AssertType(this.chart.theme, "any");
AssertType(this.chart, "any");
AssertType(this, "any");
AssertType("micro", "string");
AssertType(o, "any");

        taStroke = "stroke" in o ? o.stroke : ta.stroke,
AssertType(taStroke, "any");
AssertType("stroke" in o ? o.stroke : ta.stroke, "any");
AssertType("stroke" in o, "boolean");
AssertType("stroke", "string");
AssertType(o, "any");
AssertType(o.stroke, "any");
AssertType(ta.stroke, "any");

        size = taFont ? g.normalizedLength(g.splitFontString(taFont).size) : 0,
AssertType(size, "any");
AssertType(taFont ? g.normalizedLength(g.splitFontString(taFont).size) : 0, "any");
AssertType(taFont, "any");
AssertType(g.normalizedLength(g.splitFontString(taFont).size), "any");
AssertType(g.normalizedLength, "any");
AssertType(g.splitFontString(taFont).size, "any");
AssertType(g.splitFontString(taFont), "any");
AssertType(g.splitFontString, "any");
AssertType(taFont, "any");
AssertType(0, "int");

        cosr = Math.abs(Math.cos(rotation * Math.PI / 180)),
AssertType(cosr, "number");
AssertType(Math.abs(Math.cos(rotation * Math.PI / 180)), "number");
AssertType(Math.abs, "(number) => number");
AssertType(Math.cos(rotation * Math.PI / 180), "number");
AssertType(Math.cos, "(number) => number");
AssertType(rotation * Math.PI / 180, "number");
AssertType(rotation * Math.PI, "number");
AssertType(rotation, "number");
AssertType(Math.PI, "number");
AssertType(180, "int");

        sinr = Math.abs(Math.sin(rotation * Math.PI / 180)),
AssertType(sinr, "number");
AssertType(Math.abs(Math.sin(rotation * Math.PI / 180)), "number");
AssertType(Math.abs, "(number) => number");
AssertType(Math.sin(rotation * Math.PI / 180), "number");
AssertType(Math.sin, "(number) => number");
AssertType(rotation * Math.PI / 180, "number");
AssertType(rotation * Math.PI, "number");
AssertType(rotation, "number");
AssertType(Math.PI, "number");
AssertType(180, "int");

        tsize = taTitleFont ? g.normalizedLength(g.splitFontString(taTitleFont).size) : 0;
AssertType(tsize, "any");
AssertType(taTitleFont ? g.normalizedLength(g.splitFontString(taTitleFont).size) : 0, "any");
AssertType(taTitleFont, "any");
AssertType(g.normalizedLength(g.splitFontString(taTitleFont).size), "any");
AssertType(g.normalizedLength, "any");
AssertType(g.splitFontString(taTitleFont).size, "any");
AssertType(g.splitFontString(taTitleFont), "any");
AssertType(g.splitFontString, "any");
AssertType(taTitleFont, "any");
AssertType(0, "int");

    if (rotation < 0) {
AssertType(rotation < 0, "boolean");
AssertType(rotation, "number");
AssertType(0, "int");

        rotation += 360;
AssertType(rotation += 360, "number");
AssertType(rotation, "number");
AssertType(360, "int");
    }
    let cachedLabelW = this._getMaxLabelSize();
AssertType(cachedLabelW, "any");
AssertType(this._getMaxLabelSize(), "any");
AssertType(this._getMaxLabelSize, "any");
AssertType(this, "any");

    cachedLabelW = cachedLabelW && cachedLabelW.majLabelW;
AssertType(cachedLabelW = cachedLabelW && cachedLabelW.majLabelW, "any");
AssertType(cachedLabelW, "any");
AssertType(cachedLabelW && cachedLabelW.majLabelW, "any");
AssertType(cachedLabelW, "any");
AssertType(cachedLabelW.majLabelW, "any");

    titleOffset = size * cosr + (cachedLabelW || 0) * sinr + labelGap + Math.max(taMajorTick.length > 0 ? taMajorTick.length : 0,
AssertType(titleOffset = size * cosr + (cachedLabelW || 0) * sinr + labelGap + Math.max(taMajorTick.length > 0 ? taMajorTick.length : 0,        taMinorTick.length > 0 ? taMinorTick.length : 0) +        tsize + taTitleGap, "any");
AssertType(titleOffset, "any");
AssertType(size * cosr + (cachedLabelW || 0) * sinr + labelGap + Math.max(taMajorTick.length > 0 ? taMajorTick.length : 0,        taMinorTick.length > 0 ? taMinorTick.length : 0) +        tsize + taTitleGap, "any");
AssertType(size * cosr + (cachedLabelW || 0) * sinr + labelGap + Math.max(taMajorTick.length > 0 ? taMajorTick.length : 0,        taMinorTick.length > 0 ? taMinorTick.length : 0) +        tsize, "any");
AssertType(size * cosr + (cachedLabelW || 0) * sinr + labelGap + Math.max(taMajorTick.length > 0 ? taMajorTick.length : 0,        taMinorTick.length > 0 ? taMinorTick.length : 0), "any");
AssertType(size * cosr + (cachedLabelW || 0) * sinr + labelGap, "any");
AssertType(size * cosr + (cachedLabelW || 0) * sinr, "number");
AssertType(size * cosr, "number");
AssertType(size, "any");
AssertType(cosr, "number");
AssertType((cachedLabelW || 0) * sinr, "number");
AssertType((cachedLabelW || 0), "any");
AssertType(cachedLabelW || 0, "any");
AssertType(cachedLabelW, "any");
AssertType(0, "int");
AssertType(sinr, "number");
AssertType(labelGap, "any");
AssertType(Math.max(taMajorTick.length > 0 ? taMajorTick.length : 0,        taMinorTick.length > 0 ? taMinorTick.length : 0), "number");
AssertType(Math.max, "(...number[]) => number");
AssertType(taMajorTick.length > 0 ? taMajorTick.length : 0, "any");
AssertType(taMajorTick.length > 0, "boolean");
AssertType(taMajorTick.length, "any");
AssertType(0, "int");
AssertType(taMajorTick.length, "any");
AssertType(0, "int");

        taMinorTick.length > 0 ? taMinorTick.length : 0) +
AssertType(taMinorTick.length > 0 ? taMinorTick.length : 0, "any");
AssertType(taMinorTick.length > 0, "boolean");
AssertType(taMinorTick.length, "any");
AssertType(0, "int");
AssertType(taMinorTick.length, "any");
AssertType(0, "int");

        tsize + taTitleGap;
AssertType(tsize, "any");
AssertType(taTitleGap, "any");

    axisVector = { x: isRtl ? -1 : 1, y: 0 };     // chart mirroring
AssertType(axisVector = { x: isRtl ? -1 : 1, y: 0 }, "{ x: number; y: number; }");
AssertType(axisVector, "any");
AssertType({ x: isRtl ? -1 : 1, y: 0 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(isRtl ? -1 : 1, "union");
AssertType(isRtl, "any");
AssertType(-1, "int");
AssertType(1, "int");
AssertType(1, "int");
AssertType(y, "number");
AssertType(0, "int");

    switch (rotation) {
AssertType(rotation, "number");

        default:
            if (rotation < (90 - centerAnchorLimit)) {
AssertType(rotation < (90 - centerAnchorLimit), "boolean");
AssertType(rotation, "number");
AssertType((90 - centerAnchorLimit), "number");
AssertType(90 - centerAnchorLimit, "number");
AssertType(90, "int");
AssertType(centerAnchorLimit, "any");

                labelOffset.y = leftBottom ? size : 0;
AssertType(labelOffset.y = leftBottom ? size : 0, "any");
AssertType(labelOffset.y, "any");
AssertType(leftBottom ? size : 0, "any");
AssertType(leftBottom, "boolean");
AssertType(size, "any");
AssertType(0, "int");

            } else if (rotation < (90 + centerAnchorLimit)) {
AssertType(rotation < (90 + centerAnchorLimit), "boolean");
AssertType(rotation, "number");
AssertType((90 + centerAnchorLimit), "any");
AssertType(90 + centerAnchorLimit, "any");
AssertType(90, "int");
AssertType(centerAnchorLimit, "any");

                labelOffset.x = -size * 0.4;
AssertType(labelOffset.x = -size * 0.4, "number");
AssertType(labelOffset.x, "any");
AssertType(-size * 0.4, "number");
AssertType(-size, "number");
AssertType(size, "any");
AssertType(0.4, "double");

            } else if (rotation < 180) {
AssertType(rotation < 180, "boolean");
AssertType(rotation, "number");
AssertType(180, "int");

                labelOffset.y = leftBottom ? 0 : -size;
AssertType(labelOffset.y = leftBottom ? 0 : -size, "number");
AssertType(labelOffset.y, "any");
AssertType(leftBottom ? 0 : -size, "number");
AssertType(leftBottom, "boolean");
AssertType(0, "int");
AssertType(-size, "number");
AssertType(size, "any");

            } else if (rotation < (270 - centerAnchorLimit)) {
AssertType(rotation < (270 - centerAnchorLimit), "boolean");
AssertType(rotation, "number");
AssertType((270 - centerAnchorLimit), "number");
AssertType(270 - centerAnchorLimit, "number");
AssertType(270, "int");
AssertType(centerAnchorLimit, "any");

                labelOffset.y = leftBottom ? 0 : -size;
AssertType(labelOffset.y = leftBottom ? 0 : -size, "number");
AssertType(labelOffset.y, "any");
AssertType(leftBottom ? 0 : -size, "number");
AssertType(leftBottom, "boolean");
AssertType(0, "int");
AssertType(-size, "number");
AssertType(size, "any");

            } else if (rotation < (270 + centerAnchorLimit)) {
AssertType(rotation < (270 + centerAnchorLimit), "boolean");
AssertType(rotation, "number");
AssertType((270 + centerAnchorLimit), "any");
AssertType(270 + centerAnchorLimit, "any");
AssertType(270, "int");
AssertType(centerAnchorLimit, "any");

                labelOffset.y = leftBottom ? size * 0.4 : 0;
AssertType(labelOffset.y = leftBottom ? size * 0.4 : 0, "number");
AssertType(labelOffset.y, "any");
AssertType(leftBottom ? size * 0.4 : 0, "number");
AssertType(leftBottom, "boolean");
AssertType(size * 0.4, "number");
AssertType(size, "any");
AssertType(0.4, "double");
AssertType(0, "int");

            } else {
                labelOffset.y = leftBottom ? size : 0;
AssertType(labelOffset.y = leftBottom ? size : 0, "any");
AssertType(labelOffset.y, "any");
AssertType(leftBottom ? size : 0, "any");
AssertType(leftBottom, "boolean");
AssertType(size, "any");
AssertType(0, "int");
            }
    }

    titleRotation = (taTitleOrientation && taTitleOrientation == "away") ? 180 : 0;
AssertType(titleRotation = (taTitleOrientation && taTitleOrientation == "away") ? 180 : 0, "union");
AssertType(titleRotation, "number");
AssertType((taTitleOrientation && taTitleOrientation == "away") ? 180 : 0, "union");
AssertType((taTitleOrientation && taTitleOrientation == "away"), "any");
AssertType(taTitleOrientation && taTitleOrientation == "away", "any");
AssertType(taTitleOrientation, "any");
AssertType(taTitleOrientation == "away", "boolean");
AssertType(taTitleOrientation, "any");
AssertType("away", "string");
AssertType(180, "int");
AssertType(0, "int");

    titlePos.y = offsets.t - titleOffset + (titleRotation ? 0 : tsize);
AssertType(titlePos.y = offsets.t - titleOffset + (titleRotation ? 0 : tsize), "any");
AssertType(titlePos.y, "any");
AssertType(offsets.t - titleOffset + (titleRotation ? 0 : tsize), "any");
AssertType(offsets.t - titleOffset, "number");
AssertType(offsets.t, "any");
AssertType(titleOffset, "any");
AssertType((titleRotation ? 0 : tsize), "any");
AssertType(titleRotation ? 0 : tsize, "any");
AssertType(titleRotation, "number");
AssertType(0, "int");
AssertType(tsize, "any");

    switch (labelAlign) {
AssertType(labelAlign, "any");

        case "start":
AssertType("start", "string");

            labelAlign = "end";
AssertType(labelAlign = "end", "string");
AssertType(labelAlign, "any");
AssertType("end", "string");

            break;
        case "end":
AssertType("end", "string");

            labelAlign = "start";
AssertType(labelAlign = "start", "string");
AssertType(labelAlign, "any");
AssertType("start", "string");

            break;
        case "middle":
AssertType("middle", "string");

            labelOffset.y -= size;
AssertType(labelOffset.y -= size, "number");
AssertType(labelOffset.y, "any");
AssertType(size, "any");

            break;
    }

    let _ = rotation;
AssertType(_, "number");
AssertType(rotation, "number");
}


