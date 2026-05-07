/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

const TWO: number = 2;
const THREE: number = 3;
const FOUR: number = 4;
const EIGHT: number = 8;

function print(msg: string): void {
  console.log(msg);
}

export class SourceFile {
  fileName: string;
  fileNames = Array<string>();
  fileNumber: number;

  constructor(fileNam: string, fileNams: Array<string> | null, fileNumbe: number | null) {
    this.fileName = fileNam;
    let fileNum = this.fileNames.indexOf(fileNam);
    if (fileNum === undefined) {
      this.fileNames.push(this.fileName);
      fileNum = this.fileNames.length;
    } else {
      fileNum += 1; // File numbers are 1 based
    }
    this.fileNumber = fileNum;
  }

  name(): string {
    return this.fileName;
  }
}

/*
 * Define CodeOrigin class for w file path data processing
 */
export class CodeOrigin {
  sourceFile: SourceFile;
  lineNumber: number;

  constructor(sourceFil: SourceFile, lineNumbe: number) {
    this.sourceFile = sourceFil;
    this.lineNumber = lineNumbe;
  }

  fileName(): string {
    return this.sourceFile.name();
  }

  toString(): string {
    return this.fileName() + ':' + String(this.lineNumber);
  }
}

export class Node {
  codeOrigin: CodeOrigin | null;
  isAddress: boolean = false;
  isLabel: boolean = false;
  isImmediate: boolean = false;
  isImmediateOperand: boolean = false;
  isRegister: boolean = false;

  constructor(codeOrig: CodeOrigin | null) {
    this.codeOrigin = codeOrig;
  }

  codeOriginString(): string {
    return this.codeOrigin != null ? this.codeOrigin.toString() : '';
  }

  children(): Array<Node | null> {
    return new Array<Node | null>();
  }

  dump(): string {
    return this.codeOriginString();
  }

  value(): string {
    return '';
  }
}

/*
 * NoChild class, that is, leaf node of tree structure
 */
export class NoChildren extends Node {
  children(): Array<Node> {
    return new Array<Node>();
  }
}

function structOffsetKey(astStruct: string, field: string): string {
  return astStruct + '::' + field;
}

export class StructOffset extends NoChildren {
  static mapping = new Map<string, StructOffset>();
  astStruct: string;
  field: string;

  constructor(codeOrigin: CodeOrigin, astStruc: string, fiel: string) {
    super(codeOrigin);
    this.astStruct = astStruc;
    this.field = fiel;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isRegister = false;
  }

  static forField(codeOrigin: CodeOrigin, astStruct: string, field: string): StructOffset | null {
    let key = structOffsetKey(astStruct, field);
    if (StructOffset.mapping.get(key) === undefined) {
      StructOffset.mapping.set(key, new StructOffset(codeOrigin, astStruct, field));
    }
    return StructOffset.mapping.get(key)!;
  }

  static resetMappings(): void {
    StructOffset.mapping = new Map();
  }

  dump(): string {
    return this.astStruct + '::' + this.field;
  }
}

export class Sizeof extends NoChildren {
  static mapping = new Map<string, Sizeof>();
  astStruct: string;

  constructor(codeOrigin: CodeOrigin, astStruc: string) {
    super(codeOrigin);
    this.astStruct = astStruc;
  }

  static forName(codeOrigin: CodeOrigin, astStruct: string): Sizeof {
    if (Sizeof.mapping.get(astStruct) === undefined) {
      Sizeof.mapping.set(astStruct, new Sizeof(codeOrigin, astStruct));
    }
    return Sizeof.mapping.get(astStruct)!;
  }

  static resetMappings(): void {
    Sizeof.mapping = new Map();
  }

  dump(): string {
    return 'sizeof ' + this.astStruct;
  }
}

export class Immediate extends NoChildren {
  values: number;

  constructor(codeOrigin: CodeOrigin | null, valu: number) {
    super(codeOrigin);
    this.values = valu;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = true;
    this.isRegister = false;
  }

  dump(): string {
    return this.values.toString();
  }

  equals(other: Immediate): boolean {
    return other.values === this.values;
  }
}

export class AddImmediates extends Node {
  left: Node | null;
  right: Node | null;

  constructor(codeOrigin: CodeOrigin, lef: Node | null, righ: Node | null) {
    super(codeOrigin);
    this.left = lef;
    this.right = righ;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = true;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.left, this.right];
  }

  dump(): string {
    return '(' + this.left!.dump() + ' + ' + this.right!.dump() + ')';
  }

  value(): string {
    return this.left!.value() + this.right!.value();
  }
}

export class SubImmediates extends Node {
  left: Node | null;
  right: Node | null;

  constructor(codeOrigin: CodeOrigin, lef: Node | null, righ: Node | null) {
    super(codeOrigin);
    this.left = lef;
    this.right = righ;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = true;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.left, this.right];
  }

  dump(): string {
    return '(' + this.left!.dump() + ' - ' + this.right!.dump() + ')';
  }

  value(): string {
    return this.left!.value() + '-' + this.right!.value();
  }
}

export class MulImmediates extends Node {
  left: Node | null;
  right: Node | null;

  constructor(codeOrigin: CodeOrigin, lef: Node | null, righ: Node | null) {
    super(codeOrigin);
    this.left = lef;
    this.right = righ;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = false;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.left, this.right];
  }

  dump(): string {
    return '(' + this.left!.dump() + ' * ' + this.right!.dump() + ')';
  }
}

export class NegImmediate extends Node {
  child: Node | null;

  constructor(codeOrigin: CodeOrigin, chil: Node | null) {
    super(codeOrigin);
    this.child = chil;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = false;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.child];
  }

  dump(): string {
    return '(-' + this.child!.dump() + ')';
  }
}

export class OrImmediates extends Node {
  left: Node | null;
  right: Node | null;

  constructor(codeOrigin: CodeOrigin, lef: Node | null, righ: Node | null) {
    super(codeOrigin);
    this.left = lef;
    this.right = righ;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = false;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.left, this.right];
  }

  dump(): string {
    return '(' + this.left!.dump() + ' | ' + this.right!.dump() + ')';
  }
}

export class AndImmediates extends Node {
  left: Node | null;
  right: Node | null;

  constructor(codeOrigin: CodeOrigin, lef: Node | null, righ: Node | null) {
    super(codeOrigin);
    this.left = lef;
    this.right = righ;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = false;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.left, this.right];
  }

  dump(): string {
    return '(' + this.left!.dump() + ' & ' + this.right!.dump() + ')';
  }
}

export class XorImmediates extends Node {
  left: Node | null;
  right: Node | null;

  constructor(codeOrigin: CodeOrigin, lef: Node | null, righ: Node | null) {
    super(codeOrigin);
    this.left = lef;
    this.right = righ;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = false;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.left, this.right];
  }

  dump(): string {
    return '(' + this.left!.dump() + ' ^ ' + this.right!.dump() + ')';
  }
}

export class BitnotImmediate extends Node {
  child: Node | null;

  constructor(codeOrigin: CodeOrigin, chil: Node | null) {
    super(codeOrigin);
    this.child = chil;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = true;
    this.isImmediateOperand = false;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.child];
  }

  dump(): string {
    return '(~' + this.child!.dump() + ')';
  }
}

export class StringLiteral extends NoChildren {
  values: string;

  constructor(codeOrigin: CodeOrigin, valu: string) {
    super(codeOrigin);
    this.values = valu.slice(1, -1);

    if (!valu.startsWith('"') || !valu.endsWith('"')) {
      print('Bad string literal ' + this.values + ' at ' + this.codeOriginString());
    }

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = false;
    this.isImmediateOperand = false;
    this.isRegister = false;
  }

  dump(): string {
    return '"' + this.values + '"';
  }

  equals(other: StringLiteral): boolean {
    return other.values === this.values;
  }
}

export class RegisterId extends NoChildren {
  static mapping = new Map<string, RegisterId>();
  name: string;

  constructor(codeOrigin: CodeOrigin, nam: string) {
    super(codeOrigin);
    this.name = nam;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = false;
    this.isRegister = true;
  }

  static forName(codeOrigin: CodeOrigin, name: string): RegisterId {
    if (!RegisterId.mapping.get(name)) {
      RegisterId.mapping.set(name, new RegisterId(codeOrigin, name));
    }
    return RegisterId.mapping.get(name)!;
  }

  dump(): string {
    return this.name;
  }
}

export class FpRegisterId extends NoChildren {
  static mapping = new Map<string, FpRegisterId>();
  name: string;

  constructor(codeOrigin: CodeOrigin, nam: string) {
    super(codeOrigin);
    this.name = nam;

    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = false;
    this.isImmediateOperand = false;
    this.isRegister = true;
  }

  static forName(codeOrigin: CodeOrigin, name: string): FpRegisterId {
    if (FpRegisterId.mapping.get(name) === undefined) {
      FpRegisterId.mapping.set(name, new FpRegisterId(codeOrigin, name));
    }
    return FpRegisterId.mapping.get(name)!;
  }

  dump(): string {
    return this.name;
  }
}

export class SpecialRegister extends NoChildren {
  name: string;

  constructor(nam: string) {
    super(null);
    this.name = nam;
    this.isAddress = false;
    this.isLabel = false;
    this.isImmediate = false;
    this.isImmediateOperand = false;
    this.isRegister = true;
  }
}

export class Variable extends NoChildren {
  static mapping = new Map<string, Variable>();
  name: string;

  constructor(codeOrigin: CodeOrigin, nam: string) {
    super(codeOrigin);
    this.name = nam;
  }

  static forName(codeOrigin: CodeOrigin, name: string): Variable {
    if (Variable.mapping.get(name) === undefined) {
      Variable.mapping.set(name, new Variable(codeOrigin, name));
    }
    return Variable.mapping.get(name)!;
  }

  dump(): string {
    return this.name;
  }

  inspect(): string {
    return '<variable ' + this.name + ' at ' + this.codeOriginString();
  }
}

export class Address extends Node {
  base: Node;
  offset: Node | null;

  constructor(codeOrigin: CodeOrigin | null, bas: Node, offse: Node | null) {
    super(codeOrigin);
    this.base = bas;
    this.offset = offse;
    if (!(this.base instanceof Variable) && !this.base.isRegister) {
      print('Bad base for address ' + this.base!.codeOriginString() + ' at ' + codeOrigin!.toString());
    }
    if (!(this.offset instanceof Variable) && !this.offset!.isImmediate) {
      print('Bad offset for address ' + this.offset!.codeOriginString() + ' at ' + codeOrigin!.toString());
    }

    this.isAddress = true;
    this.isLabel = false;
    this.isImmediate = false;
    this.isImmediateOperand = true;
    this.isRegister = false;
  }

  children(): Array<Node | null> {
    return [this.base, this.offset];
  }

  dump(): string {
    return this.offset!.dump() + '[' + this.base.dump() + ']';
  }
}

export class BaseIndex extends Node {
  base: Node;
  index: Node;
  scale: number;
  offset: Node | null;

  constructor(codeOrigin: CodeOrigin | null, bas: Node, inde: Node, scal: number, offse: Node | null) {
    super(codeOrigin);
    this.base = bas;
    this.index = inde;
    this.scale = scal;
    this.offset = offse;

    if (![1, TWO, FOUR, EIGHT].includes(this.scale)) {
      print('Bad scale ' + this.scale + ' at ' + this.codeOriginString());
    }

    this.isAddress = true;
    this.isLabel = false;
    this.isImmediate = false;
    this.isImmediateOperand = false;
    this.isRegister = false;
  }

  scaleShift(): number | null {
    switch (this.scale) {
      case 1:
        return 0;
      case TWO:
        return 1;
      case FOUR:
        return TWO;
      case EIGHT:
        return THREE;
      default:
        print('Bad scale ' + this.scale + ' at ' + this.codeOriginString());
        return null;
    }
  }

  children(): Array<Node | null> {
    return [this.base, this.index, this.offset];
  }

  dump(): string {
    return this.offset!.dump() + '[' + this.base.dump() + ', ' + this.index.dump() + ', ' + this.scale + ']';
  }
}

export class AbsoluteAddress extends NoChildren {
  address: Node | null;

  constructor(codeOrigin: CodeOrigin | null, addres: Node | null) {
    super(codeOrigin);
    this.address = addres;

    this.isAddress = true;
    this.isLabel = false;
    this.isImmediate = false;
    this.isImmediateOperand = true;
    this.isRegister = false;
  }

  dump(): string {
    return this.address!.dump() + '[]';
  }
}

export class Instruction extends Node {
  opcode: string;
  operands: Array<Node | null>;
  annotation: string | null;

  constructor(codeOrigin: CodeOrigin, opcod: string, operand: Array<Node | null>, annotatio: string | null) {
    super(codeOrigin);
    this.opcode = opcod;
    this.operands = operand;
    this.annotation = annotatio;
  }

  children(): Array<Node | null> {
    return Array<Node | null>();
  }

  dump(): string {
    return (
      '    ' +
      this.opcode +
      ' ' +
      this.operands
        .map((v: Node | null): string => {
          return v!.dump();
        })
        .join(', ')
    );
  }
}

export class Error extends NoChildren {
  dump(): string {
    return '    error';
  }
}

export class ConstExpr extends NoChildren {
  values: string;
  static mapping = new Map<string, ConstExpr>();

  constructor(codeOrigin: CodeOrigin, valu: string) {
    super(codeOrigin);

    this.values = valu;
  }

  static forName(codeOrigin: CodeOrigin, text: string): NoChildren {
    if (ConstExpr.mapping.get(text) === undefined) {
      ConstExpr.mapping.set(text, new ConstExpr(codeOrigin, text));
    }
    return ConstExpr.mapping.get(text)!;
  }

  static resetMappings(): void {
    ConstExpr.mapping = new Map();
  }

  dump(): string {
    return 'constexpr (' + this.values + ')';
  }

  compare(other: ConstExpr): boolean {
    return this.values === other.values;
  }

  isImmediates(): boolean {
    return true;
  }
}

export class ConstDecl extends Node {
  variable: Variable | null;
  valueDecl: Node | null;

  constructor(codeOrigin: CodeOrigin, variabl: Variable | null, valueDec: Node | null) {
    super(codeOrigin);
    this.variable = variabl;
    this.valueDecl = valueDec;
  }

  children(): Array<Node | null> {
    return [this.variable, this.valueDecl];
  }

  dump(): string {
    return 'const ' + this.variable!.dump() + ' = ' + this.valueDecl!.dump();
  }
}

/*
 * The global variable
 */
let labelMapping = new Map<string, Node>();
let referencedExternLabels = Array<Label>();

export class Label extends NoChildren {
  name: string;
  extern: boolean = true;
  global: boolean = false;

  constructor(codeOrigin: CodeOrigin | null, nam: string) {
    super(codeOrigin);
    this.name = nam;
  }

  static forName(codeOrigin: CodeOrigin | null, name: string, definedInFile: boolean = false): Label {
    if (labelMapping.get(name) === undefined) {
      labelMapping.set(name, new Label(codeOrigin, name));
    }
    if (definedInFile) {
      (labelMapping.get(name) as Label).clearExtern();
    }
    return labelMapping.get(name) as Label;
  }

  static setAsGlobal(codeOrigin: CodeOrigin, name: string): void {
    if (labelMapping.get(name) !== undefined) {
      let label = labelMapping.get(name) as Label;
      if (label.isGlobal()) {
        print('Label: ' + name + ' declared global multiple times');
      }
      label.setGlobal();
    } else {
      let newLabel = new Label(codeOrigin, name);
      newLabel.setGlobal();
      labelMapping.set(name, newLabel);
    }
  }

  static resetMappings(): void {
    labelMapping = new Map();
    referencedExternLabels = Array<Label>();
  }

  static resetReferenced(): void {
    referencedExternLabels = Array<Label>();
  }

  clearExtern(): void {
    this.extern = false;
  }

  isExtern(): boolean {
    return this.extern;
  }

  setGlobal(): void {
    this.global = true;
  }

  isGlobal(): boolean {
    return this.global;
  }

  dump(): string {
    return this.name + ':';
  }
}

export class LocalLabel extends NoChildren {
  static uniqueNameCounter: number = 0;
  extern: boolean = true;
  name: string;

  constructor(codeOrigin: CodeOrigin | null, nam: string) {
    super(codeOrigin);
    this.name = nam;
  }

  static forName(codeOrigin: CodeOrigin | null, name: string): LocalLabel {
    if (labelMapping.get(name) === undefined) {
      labelMapping.set(name, new LocalLabel(codeOrigin, name));
    }
    return labelMapping.get(name) as LocalLabel;
  }

  unique(comment: string): LocalLabel {
    let newName = '_' + comment;
    if (labelMapping.get(newName) !== undefined) {
      newName = '_#' + LocalLabel.uniqueNameCounter + '_' + comment;
      while (labelMapping.get(newName) !== undefined) {
        LocalLabel.uniqueNameCounter++;
      }
    }
    return LocalLabel.forName(null, newName);
  }

  static resetMappings(): void {
    LocalLabel.uniqueNameCounter = 0;
  }

  cleanName(): string {
    if (this.name.startsWith('.')) {
      return '_' + this.name.slice(1);
    }
    return this.name;
  }

  isGlobal(): boolean {
    return false;
  }

  dump(): string {
    return this.name + ':';
  }
}

export class LabelReference extends Node {
  label: Label;

  constructor(codeOrigin: CodeOrigin, labe: Label) {
    super(codeOrigin);
    this.label = labe;

    this.isAddress = false;
    this.isLabel = true;
    this.isImmediate = false;
    this.isImmediateOperand = true;
  }

  children(): Array<Node> {
    return [this.label];
  }

  name(): string {
    return this.label.name;
  }

  isExtern(): boolean {
    let node = labelMapping.get(this.name())
    if(node && node instanceof LabelReference) {
      return node.isExtern()
    }
    return false
  }

  used(): void {
    if (!referencedExternLabels.includes(this.label) && this.isExtern()) {
      referencedExternLabels.push(this.label);
    }
  }

  dump(): string {
    return this.label.name;
  }
}

export class LocalLabelReference extends NoChildren {
  label: LocalLabel;

  constructor(codeOrigin: CodeOrigin, labe: LocalLabel) {
    super(codeOrigin);
    this.label = labe;

    this.isAddress = false;
    this.isLabel = true;
    this.isImmediate = false;
    this.isImmediateOperand = true;
  }

  children(): Array<Node> {
    return [this.label];
  }

  name(): string {
    return this.label.name;
  }

  dump(): string {
    return this.label.name;
  }
}

export class Sequence extends Node {
  list: Array<Node>;

  constructor(codeOrigin: CodeOrigin, lis: Array<Node>) {
    super(codeOrigin);
    this.list = lis;
  }

  children(): Array<Node> {
    return this.list;
  }

  dump(): string {
    return (
      '' +
      this.list
        .map((v: Node) => {
          return v.dump();
        })
        .join('\n')
    );
  }
}

export class True extends NoChildren {
  static instance(): True {
    return new True(null);
  }

  values(): boolean {
    return true;
  }

  dump(): string {
    return 'true';
  }
}

export class False extends NoChildren {
  static instance(): False {
    return new False(null);
  }

  values(): boolean {
    return false;
  }

  dump(): string {
    return 'false';
  }
}

export class Setting extends NoChildren {
  static mapping = new Map<string, Setting>();
  name: string;

  constructor(codeOrigin: CodeOrigin, nam: string) {
    super(codeOrigin);
    this.name = nam;
  }

  static forName(codeOrigin: CodeOrigin, name: string): Setting {
    if (Setting.mapping.get(name) === undefined) {
      Setting.mapping.set(name, new Setting(codeOrigin, name));
    }
    return Setting.mapping.get(name)!;
  }

  static resetMappings(): void {
    Setting.mapping = new Map();
  }

  dump(): string {
    return this.name;
  }
}

export class And extends Node {
  left: Node | null;
  right: Node;

  constructor(codeOrigin: CodeOrigin, lef: Node | null, righ: Node) {
    super(codeOrigin);
    this.left = lef;
    this.right = righ;
  }

  children(): Array<Node | null> {
    return [this.left, this.right];
  }

  dump(): string {
    return '(' + this.left!.dump() + ' and ' + this.right!.dump() + ')';
  }
}

export class Or extends Node {
  left: Node;
  right: Node;

  constructor(codeOrigin: CodeOrigin, lef: Node, righ: Node) {
    super(codeOrigin);
    this.left = lef;
    this.right = righ;
  }

  children(): Array<Node> {
    return [this.left, this.right];
  }

  dump(): string {
    return '(' + this.left.dump() + ' or ' + this.right.dump() + ')';
  }
}

export class Not extends Node {
  child: Node;

  constructor(codeOrigin: CodeOrigin, chil: Node) {
    super(codeOrigin);
    this.child = chil;
  }

  children(): Array<Node> {
    return [this.child];
  }

  dump(): string {
    return '(not' + this.child.dump() + ')';
  }
}

export class Skip extends NoChildren {
  dump(): string {
    return '    skip';
  }
}

export class IfThenElse extends Node {
  predicate: Node | null;
  thenCase: Node;
  elseCase: Node;

  constructor(codeOrigin: CodeOrigin, predicat: Node | null, thenCas: Node) {
    super(codeOrigin);
    this.predicate = predicat;
    this.thenCase = thenCas;
    this.elseCase = new Skip(codeOrigin);
  }

  dump(): string {
    return 'if ' + this.predicate!.dump() + '\n' + this.thenCase.dump() + '\nelse\n' + this.elseCase.dump() + '\nend';
  }
}

export class Macro extends Node {
  name: string;
  variables: Array<Variable>;
  body: Node;

  constructor(codeOrigin: CodeOrigin, nam: string, variable: Array<Variable>, bod: Node) {
    super(codeOrigin);
    this.name = nam;
    this.variables = variable;
    this.body = bod;
  }

  dump(): string {
    return (
      'macro ' +
      this.name +
      '(' +
      this.variables
        .map((v: Node): string => {
          return v.dump();
        })
        .join(', ') +
      ')\n' +
      this.body.dump() +
      '\nend'
    );
  }
}

export class MacroCall extends Node {
  name: string;
  operands: Array<Node | null>;
  annotation: string | null;

  constructor(codeOrigin: CodeOrigin, nam: string, operand: Array<Node | null>, annotatio: string | null) {
    super(codeOrigin);
    this.name = nam;
    this.operands = operand;
    this.annotation = annotatio;
  }

  children(): Array<Node | null> {
    return Array<Node | null>();
  }

  dump(): string {
    return (
      '    ' +
      this.name +
      '(' +
      this.operands
        .map((v: Node | null): string => {
          return v!.dump();
        })
        .join(', ') +
      ')'
    );
  }
}

/*
 * Reset the static mapping property of the following node classes
 */
export function resetAST(): void {
  StructOffset.resetMappings();
  ConstExpr.resetMappings();
  Setting.resetMappings();
  Label.resetMappings();
  Sizeof.resetMappings();
  LocalLabel.resetMappings();
}
