/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

class ArkBorderStyle {
    constructor() {
        this.type = undefined;
        this.style = undefined;
        this.top = undefined;
        this.right = undefined;
        this.bottom = undefined;
        this.left = undefined;
    }
    isEqual(another) {
        return (this.type === another.type) && (this.style === another.style) &&
            (this.top === another.top) && (this.right === another.right) &&
            (this.bottom === another.bottom) && (this.left === another.left);
    }
    parseBorderStyle(value) {
        if (typeof value === "number") {
            this.style = value;
            this.type = true;
            return true;
        }
        else if (typeof value === "object") {
            return this.parseEdgeStyles(value);
        }
        return false;
    }
    parseEdgeStyles(options) {
        this.top = options.top;
        this.right = options.right;
        this.bottom = options.bottom;
        this.left = options.left;
        this.type = true;
        return true;
    }
}
class ArkBorderColor {
    constructor() {
        this.leftColor = undefined;
        this.rightColor = undefined;
        this.topColor = undefined;
        this.bottomColor = undefined;
    }
    isEqual(another) {
        return (this.leftColor === another.leftColor && this.rightColor === another.rightColor && this.topColor === another.topColor && this.bottomColor === another.bottomColor);
    }
}
class ArkPosition {
    constructor() {
        this.x = undefined;
        this.y = undefined;
    }
    isEqual(another) {
        return (this.x === another.x) && (this.y === another.y);
    }
}
class ArkBorderWidth {
    constructor() {
        this.left = undefined;
        this.right = undefined;
        this.top = undefined;
        this.bottom = undefined;
    }
    isEqual(another) {
        return (this.left === another.left && this.right === another.right && this.top === another.top && this.bottom === another.bottom);
    }
}
class ArkBorderRadius {
    constructor() {
        this.topLeft = undefined;
        this.topRight = undefined;
        this.bottomLeft = undefined;
        this.bottomRight = undefined;
    }
    isEqual(another) {
        return (this.topLeft === another.topLeft && this.topRight === another.topRight && this.bottomLeft === another.bottomLeft && this.bottomRight === another.bottomRight);
    }
}
class ArkTransformMatrix {
    constructor(matrix) {
        this.matrix = matrix;
        this.length = 16;
    }
    compareArrays(arr1, arr2) {
        return Array.isArray(arr1)
            && Array.isArray(arr2)
            && arr1.length === arr2.length
            && arr1.every((value, index) => value === arr2[index]);
    }
    isEqual(another) {
        this.compareArrays(this.matrix, another.matrix);
    }
}
class Modifier {
    constructor(value) {
        this.stageValue = value;
    }
    applyStage(node) {
        if (this.stageValue === undefined) {
            this.applyPeer(node, true);
            return true;
        }
        const stageTypeInfo = typeof this.stageValue;
        const valueTypeInfo = typeof this.value;
        let different = false;
        if (stageTypeInfo !== valueTypeInfo) {
            different = true;
        }
        else if (stageTypeInfo === "number" || stageTypeInfo === 'string' || stageTypeInfo === 'boolean') {
            different = (this.stageValue !== this.value);
        }
        else {
            different = this.checkObjectDiff();
        }
        if (different) {
            this.value = this.stageValue;
            this.applyPeer(node, false);
        }
        this.stageValue = undefined;
        return false;
    }
    applyPeer(node, reset) { }
    checkObjectDiff() { return true; }
}
class BackgroundColorModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetBackgroundColor(node);
        }
        else {
            getArkUINativeModule().common.setBackgroundColor(node, this.value);
        }
    }
}
BackgroundColorModifier.identity = "backgroundColor";
class WidthModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetWidth(node);
        }
        else {
            getArkUINativeModule().common.setWidth(node, this.value);
        }
    }
}
WidthModifier.identity = "width";
class BorderWidthModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetBorderWidth(node);
        }
        else {
            getArkUINativeModule().common.setBorderWidth(node, this.value.left, this.value.right, this.value.top, this.value.bottom);
        }
    }
}
BorderWidthModifier.identity = "borderWidth";
class HeightModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetHeight(node);
        }
        else {
            getArkUINativeModule().common.setHeight(node, this.value);
        }
    }
}
HeightModifier.identity = "height";
class BorderRadiusModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetBorderRadius(node);
        }
        else {
            getArkUINativeModule().common.setBorderRadius(node, this.value.topLeft, this.value.topRight, this.value.bottomLeft, this.value.bottomRight);
        }
    }
}
BorderRadiusModifier.identity = "borderRadius";
class PositionModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetPosition(node);
        }
        else {
            getArkUINativeModule().common.setPosition(node, this.value.x, this.value.y);
        }
    }
}
PositionModifier.identity = "position";
class BorderColorModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetBorderColor(node);
        }
        else {
            getArkUINativeModule().common.setBorderColor(node, this.value.leftColor, this.value.rightColor, this.value.topColor, this.value.bottomColor);
        }
    }
}
BorderColorModifier.identity = "borderColor";
class BorderStyleModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetBorderStyle(node);
        }
        else {
            getArkUINativeModule().common.setBorderStyle(node, this.value.type, this.value.style, this.value.top, this.value.right, this.value.bottom, this.value.left);
        }
    }
}
BorderStyleModifier.identity = "borderStyle";
class ZIndexModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetZIndex(node);
        }
        else {
            getArkUINativeModule().common.setZIndex(node, this.value);
        }
    }
}
ZIndexModifier.identity = "zIndex";
class OpacityModifier extends Modifier {
    applyPeer(node, reset) {
        if (reset) {
            getArkUINativeModule().common.resetOpacity(node);
        }
        else {
            getArkUINativeModule().common.setOpacity(node, this.value);
        }
    }
}
OpacityModifier.identity = "opacity";
function modifier(modifiers, identity, modifierClass, value) {
    const item = modifiers.get(identity);
    if (item) {
        item.stageValue = value;
    }
    else {
        modifiers.set(identity, new modifierClass(value));
    }
}
class ArkComponent {
    constructor(nativePtr) {
        this._modifiers = new Map();
        this.nativePtr = nativePtr;
    }
    applyModifierPatch() {
        let expiringItems = [];
        this._modifiers.forEach((value, key) => {
            if (value.applyStage(this.nativePtr)) {
                expiringItems.push(key);
            }
        });
        expiringItems.forEach(key => {
            this._modifiers.delete(key);
        });
    }
    width(value) {
        modifier(this._modifiers, WidthModifier.identity, WidthModifier, value);
        return this;
    }
    height(value) {
        modifier(this._modifiers, HeightModifier.identity, HeightModifier, value);
        return this;
    }
    backgroundColor(value) {
        modifier(this._modifiers, BackgroundColorModifier.identity, BackgroundColorModifier, value);
        return this;
    }
    opacity(value) {
        modifier(this._modifiers, OpacityModifier.identity, OpacityModifier, value);
        return this;
    }
    borderStyle(value) {
        var arkBorderStyle = new ArkBorderStyle();
        if (arkBorderStyle.parseBorderStyle(value)) {
            modifier(this._modifiers, BorderStyleModifier.identity, BorderStyleModifier, arkBorderStyle);
        }
        else {
            modifier(this._modifiers, BorderStyleModifier.identity, BorderStyleModifier, undefined);
        }
        return this;
    }
    borderWidth(value) {
        var borderWidth = new ArkBorderWidth();
        if (typeof value === "number" || typeof value === "string") {
            borderWidth.left = value;
            borderWidth.right = value;
            borderWidth.top = value;
            borderWidth.bottom = value;
        }
        modifier(this._modifiers, BorderWidthModifier.identity, BorderWidthModifier, borderWidth);
        return this;
    }
    borderColor(value) {
        var borderColorGroup = new ArkBorderColor();
        if (typeof value === "number" || typeof value === "string") {
            borderColorGroup.leftColor = value;
            borderColorGroup.rightColor = value;
            borderColorGroup.topColor = value;
            borderColorGroup.bottomColor = value;
        }
        modifier(this._modifiers, BorderColorModifier.identity, BorderColorModifier, borderColorGroup);
        return this;
    }
    borderRadius(value) {
        var borderRadius = new ArkBorderRadius;
        if (typeof value === "number" || typeof value === "string") {
            borderRadius.topLeft = value;
            borderRadius.topRight = value;
            borderRadius.bottomLeft = value;
            borderRadius.bottomRight = value;
        }
        modifier(this._modifiers, BorderRadiusModifier.identity, BorderRadiusModifier, borderRadius);
        return this;
    }
    zIndex(value) {
        modifier(this._modifiers, ZIndexModifier.identity, ZIndexModifier, value);
        return this;
    }
    position(value) {
        modifier(this._modifiers, PositionModifier.identity, PositionModifier, value);
        return this;
    }
}
//@ts-nocheck
class ArkStackComponent extends ArkComponent {
    alignContent(value) {
        throw new Error("Method not implemented.");
    }
}
function attributeModifier(modifier) {
    const elmtId = ViewStackProcessor.GetElmtIdToAccountFor();
    var nativeNode = getArkUINativeModule().getFrameNodeById(elmtId);
    // var component = this.createOrGetNode(elmtId, () =>
    // {
    //     return new ArkStackComponent(nativeNode);
    // });
    let component = new ArkStackComponent(nativeNode);
    modifier.applyNormalAttribute(component);
    component.applyModifierPatch();
}
class ModifierStyle {
    applyNormalAttribute(instance) {
        instance.width(80);
        instance.height(80);
        instance.backgroundColor('red');
        instance.borderWidth(5);
        instance.zIndex(2);
        instance.borderStyle(1);
        instance.borderColor('black');
        instance.opacity(1);
    }
}
class ViewStackProcessor {
    static GetElmtIdToAccountFor() {
        return 1;
    }
}
class CommonModifierMock {
    setWidth() { }
    setHeight() { }
    setBackgroundColor() { }
    setBorderWidth() { }
    setBorderColor() { }
    setZIndex() { }
    setOpacity() { }
    setBorderStyle() { }
}
class NativeModule {
    constructor() {
        this.common = new CommonModifierMock();
    }
    getFrameNodeById(elmtId) {
        return elmtId;
    }
}
const nativeNode = new NativeModule();
function getArkUINativeModule() {
    return nativeNode;
}
function probe(tag) {
    let result = Date.now();
    //print(tag + ": " + result.toString());
    return result;
}

function main() {
    let modifierMock = new ModifierStyle();
    let total = 0;

    // startRuntimeStat();
    for (let i = 0; i < 20; i++) {
        let before = probe('build1');
        for (let i = 0; i < 1500; i++) {
            attributeModifier(modifierMock);
        }
        //ArkTools.dumpOpcodeStat();
        let after = probe('build2');
        // print((after - before));
        total = total + (after - before);
    }
    // stopRuntimeStat();
    // print(`Took ${total}ms`);

    print(`arkui: ms = ${total}`);
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
    main();
}


function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

main();