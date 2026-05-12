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

import { RandomGenerator } from './random';
import { Lexer, Token } from './lexer';


const ARRAY_DEFAULT = 11;
const RUN_NUM = 14;

const DEBUG = false;

class CaselessMap {
  private _map: Map<string, NumberValue | NumberArray | NativeFunction | string> = new Map();

  constructor(otherMap: CaselessMap | null) {
    if (otherMap !== null) {
      this._map = otherMap._map;
    } else {
      this._map = new Map();
    }
  }

  set(key: string, value: NumberValue | NumberArray | NativeFunction | string): void {
    this._map.set(key.toLowerCase(), value);
  }

  has(key: string): boolean {
    return this._map.get(key.toLowerCase()) !== undefined;
  }

  get(key: string): NumberValue | NumberArray | NativeFunction | string | undefined {
    return this._map.get(key.toLowerCase());
  }

  [Symbol.iterator](): IterableIterator<[string, string | NumberValue | NumberArray | NativeFunction]> {
    return this._map[Symbol.iterator]();
  }
}

interface Statement {
  lineNumber: number;

  sourceLineNumber: number;
}

interface VoidStatement extends Statement {
  process(state: State): void;
}

interface OutputStatement extends Statement {
  process(state: State): [kind: string, string: string];
}

class LetStatement implements VoidStatement {
  lineNumber: number;

  sourceLineNumber: number;

  variable: Variable;

  expression: NumericExpression;

  constructor(lineNumber: number, sourceLineNumber: number, expression: NumericExpression, variable: Variable) {
    this.lineNumber = lineNumber;
    this.sourceLineNumber = sourceLineNumber;
    this.expression = expression;
    this.variable = variable;
  }

  process(state: State): void {
    this.variable.evaluate(state).assign(this.expression.evaluate(state));
  }
}

class IfStatement implements VoidStatement {
  lineNumber: number;

  sourceLineNumber: number;

  target: number;

  condition: RelationalExpression;

  constructor(lineNumber: number, sourceLineNumber: number, target: number, condition: RelationalExpression) {
    this.lineNumber = lineNumber;
    this.sourceLineNumber = sourceLineNumber;
    this.target = target;
    this.condition = condition;
  }

  process(state: State): void {
    if (this.condition.evaluate(state)) {
      state.nextLineNumber = this.target;
    }
  }
}

class ForStatementStep {
  value = 1;

  evaluate(): number {
    return this.value;
  }
}

class ForStateItem {
  variable: NumberValue | null = null;

  initialValue: number | null = null;

  limitValue: number | null = null;

  stepValue: number | null = null;

  get shouldStop(): Boolean {
    return (this.variable!.value - this.limitValue!) * Math.sin(this.stepValue!) > 0;
  }
}

class ForStatement implements VoidStatement {
  lineNumber: number;

  sourceLineNumber: number;

  nextLineNumber: number | null = null;

  variable: string;

  initial: NumericExpression;

  limit: NumericExpression;

  step: NumericExpression | number;

  target: NextStatement | null = null;

  constructor(
    lineNumber: number,
    sourceLineNumber: number,
    variable: string,
    initial: NumericExpression,
    limit: NumericExpression,
    step: NumericExpression | number
  ) {
    this.lineNumber = lineNumber;
    this.sourceLineNumber = sourceLineNumber;
    this.variable = variable;
    this.initial = initial;
    this.limit = limit;
    this.step = step;
  }

  process(state: State): void {
    let sideState = state.getSideState(this);
    sideState.variable = (state.getValue(this.variable, 0) as NumberValue).leftApply(state, []);
    sideState.initialValue = this.initial.evaluate(state);
    sideState.limitValue = this.limit.evaluate(state);
    if ((this.step as NumericExpression).evaluate) {
      sideState.stepValue = (this.step as NumericExpression).evaluate(state);
    } else {
      sideState.stepValue = 1;
    }
    sideState.variable.assign(sideState.initialValue);
    if (sideState.shouldStop) {
      this.nextLineNumber = this.target!.lineNumber + 1;
    }
  }
}

class NextStatement implements VoidStatement {
  target: ForStatement | null = null;

  lineNumber: number;

  sourceLineNumber: number;

  variable: string;

  process(state: State): void {
    let sideState = state.getSideState(this.target!);
    sideState.variable!.assign(sideState.variable!.value + sideState.stepValue!);
    if (sideState.shouldStop) {
      return;
    }
    state.nextLineNumber = this.target!.lineNumber + 1;
  }

  constructor(lineNumber: number, sourceLineNumber: number, variable: string) {
    this.lineNumber = lineNumber;
    this.sourceLineNumber = sourceLineNumber;
    this.variable = variable;
  }
}

/// 打印语句
class PrintStatement implements OutputStatement {
  lineNumber: number;

  sourceLineNumber: number;

  items: [kind: string, value: Expression | null][] = new Array();

  process(state: State): [kind: string, string: string] {
    let string = '';
    for (let item of this.items) {
      switch (item[0]) {
        case 'comma':
          while (string.length % RUN_NUM !== 0) {
            string += ' ';
          }
          break;
        case 'string':
          string += (item[1] as StringExpression).evaluate(state);
          break;
        case 'number':
          string += `${(item[1] as NumericExpression).evaluate(state)}`;
          break;
        default:
          throw new Error('Bad item kind: (item.kind)');
      }
    }
    return ['output', string];
  }

  constructor(lineNumber: number, sourceLineNumber: number) {
    this.lineNumber = lineNumber;
    this.sourceLineNumber = sourceLineNumber;
  }
}

class DimStatement implements VoidStatement {
  lineNumber: number;

  sourceLineNumber: number;

  items: [name: string, bounds: number[]][] = new Array();

  constructor(lineNumber: number, sourceLineNumber: number) {
    this.lineNumber = lineNumber;
    this.sourceLineNumber = sourceLineNumber;
  }

  process(state: State): void {
    for (let [name, bounds] of this.items) {
      let vals = state.values as CaselessMap;
      state.validate(!vals.has(name), `Variable ${name} already exists`);
      state.validate(bounds.length > 0, 'Dim statement is for arrays');
      vals.set(name, new NumberArray(bounds.map(e => e + 1)));
    }
  }
}

class EndStatement implements VoidStatement {
  lineNumber: number;

  sourceLineNumber: number;

  constructor(lineNumber: number, sourceLineNumber: number) {
    this.lineNumber = lineNumber;
    this.sourceLineNumber = sourceLineNumber;
  }

  process(state: State): void {
    state.nextLineNumber = undefined;
  }
}

class State {
  values: CaselessMap;

  stringValues: CaselessMap;

  sideState: WeakMap<ForStatement, ForStateItem>;

  statement?: Statement;

  nextLineNumber?: number;

  program: Program;

  rngGenner: RandomGenerator;

  constructor(program: Program) {
    this.values = new CaselessMap(null);
    this.stringValues = new CaselessMap(null);
    this.sideState = new WeakMap();
    this.nextLineNumber = 0;
    this.program = program;
    this.rngGenner = new RandomGenerator();
    this.addNative('abs', x => Math.abs(x[0]));
    this.addNative('atn', x => Math.atan(x[0]));
    this.addNative('cos', x => Math.cos(x[0]));
    this.addNative('exp', x => Math.exp(x[0]));
    this.addNative('int', x => Math.floor(x[0]));
    this.addNative('log', x => Math.log(x[0]));
    this.addNative('rnd', x => this.rngGenner.createRNGWithFixedSeed()());
    this.addNative('sgn', x => Math.sign(x[0]));
    this.addNative('sin', x => Math.sin(x[0]));
    this.addNative('sqr', x => Math.sqrt(x[0]));
    this.addNative('tan', x => Math.tan(x[0]));
  }

  addNative(name: string, callback: (num: number[]) => number): void {
    (this.values as CaselessMap).set(name, new NativeFunction(callback));
  }

  getValue(name: string, numParameters: number): NumberValue | NumberArray | NativeFunction {
    let values = this.values as CaselessMap;
    let numParametersLocal = numParameters;
    if (values.has(name)) {
      return values.get(name) as NumberValue | NumberArray | NativeFunction;
    }

    let result: NumberValue | NumberArray | NativeFunction;
    if (numParametersLocal === 0) {
      result = new NumberValue();
    } else {
      let dim: number[] = [];
      while (numParametersLocal > 0) {
        numParametersLocal -= 1;
        dim.push(ARRAY_DEFAULT);
      }
      result = new NumberArray(dim);
    }
    values.set(name, result!);
    return result;
  }

  getSideState(key: ForStatement): ForStateItem {
    if (this.sideState.get(key) === undefined) {
      let result = new ForStateItem();
      this.sideState.set(key, result);
      return result;
    }
    return this.sideState.get(key)!;
  }

  abort(text: string): void {
    if (this.statement === null) {
      throw new Error(`At beginning of execution: ${text}`);
    }
    throw new Error('At ' + this.statement!.sourceLineNumber + ': ' + text);
  }

  validate(predicate: boolean, text: string): void {
    if (!predicate) {
      this.abort(text);
    }
  }
}

/// 数字元素
class NumberValue {
  _value: number;

  get value(): number {
    return this._value;
  }

  set value(newValue: number) {
    this._value = newValue;
  }

  constructor(value: number = 0) {
    this._value = value;
  }

  apply(state: State, parameters: number[]): number {
    state.validate(parameters.length === 0, 'Should not pass arguments to simple numeric variables');
    return this._value;
  }

  leftApply(state: State, parameters: number[]): NumberValue {
    state.validate(parameters.length === 0, 'Should not pass arguments to simple numeric variables');
    return this;
  }

  assign(value: number): void {
    this._value = value;
  }
}

type ArrayItem = NumberValue | ArrayItem[];
// 多维数组
class NumberArray {
  _dim: number[];

  _array: ArrayItem[]; /// NumberValue | NumberArray

  constructor(dim: number[] = [ARRAY_DEFAULT]) {
    function allocateDim(index: number): ArrayItem[] {
      let result = new Array<ArrayItem>(dim[index]);
      if (index + 1 < dim.length) {
        for (let i = 0; i < dim[index]; i++) {
          result[i] = allocateDim(index + 1);
        }
      } else {
        for (let i = 0; i < dim[index]; i++) {
          result[i] = new NumberValue();
        }
      }
      return result;
    }

    this._array = allocateDim(0);
    this._dim = dim;
  }

  apply(state: State, parameters: number[]): number {
    return this.leftApply(state, parameters).apply(state, []);
  }

  leftApply(state: State, parameters: number[]): NumberValue {
    if (this._dim.length !== parameters.length) {
      state.abort(`Expected ${this._dim.length} arguments but ${parameters.length} were passed.`);
    }

    let result: ArrayItem[] | ArrayItem = this._array;
    for (let i = 0; i < parameters.length; i++) {
      let index = Math.floor(parameters[i]);
      if (!(index >= state.program.base) || !(index < (result as ArrayItem[]).length)) {
        state.abort(`Index out of bounds: ${index}`);
      }
      result = (result as ArrayItem[])[index];
    }
    return result as NumberValue;
  }
}

class NativeFunction {
  _callback: (num: number[]) => number;

  constructor(callback: (num: number[]) => number) {
    this._callback = callback;
  }

  apply(state: State, parameters: number[]): number {
    return this._callback(parameters);
  }

  leftApply(state: State, parameters?: number): void {
    state.abort('Cannot use a native function as an lvalue');
  }
}

interface Expression {}

/// 数字表达式
interface NumericExpression extends Expression {
  evaluate(state: State): number;
}

/// 字符串表达式
interface StringExpression extends Expression {
  evaluate(state: State): string;
}

class PrimaryExpression implements NumericExpression {
  name: string;

  parameters: NumericExpression[];

  constructor(name: string, parameters: NumericExpression[]) {
    this.name = name;
    this.parameters = parameters;
  }

  evaluate(state: State): number {
    let parameters = this.parameters.map(e => e.evaluate(state));
    let fun = state.getValue(this.name, parameters.length);

    if (fun instanceof NumberValue) {
      let num = fun as NumberValue;
      return num.apply(state, parameters);
    }
    if (fun instanceof NumberArray) {
      let arr = fun as NumberArray;
      return arr.apply(state, parameters);
    }
    if (fun instanceof NativeFunction) {
      let nf = fun as NativeFunction;
      return nf.apply(state, parameters);
    }
    return 0;
  }
}

/// Factor pow表达式
class FactorExpression implements NumericExpression {
  left: NumericExpression;

  right: NumericExpression;

  evaluate(state: State): number {
    let lf = this.left.evaluate(state);
    let rg = this.right.evaluate(state);
    return Math.pow(lf, rg);
  }

  constructor(left: NumericExpression, right: NumericExpression) {
    this.left = left;
    this.right = right;
  }
}

export class NumberNegExpression implements NumericExpression {
  term: NumericExpression;

  evaluate(state: State): number {
    return -this.term.evaluate(state);
  }

  constructor(term: NumericExpression) {
    this.term = term;
  }
}

export class NumberAddExpression implements NumericExpression {
  left: NumericExpression;

  right: NumericExpression;

  constructor(left: NumericExpression, right: NumericExpression) {
    this.left = left;
    this.right = right;
  }

  evaluate(state: State): number {
    let lf = this.left.evaluate(state);
    let rg = this.right.evaluate(state);
    return lf + rg;
  }
}

class NumberSubExpression implements NumericExpression {
  left: NumericExpression;

  right: NumericExpression;

  constructor(left: NumericExpression, right: NumericExpression) {
    this.left = left;
    this.right = right;
  }

  evaluate(state: State): number {
    let lf = this.left.evaluate(state);
    let rg = this.right.evaluate(state);
    return lf - rg;
  }
}

export class Variable {
  name: string;

  parameters: NumericExpression[];

  constructor(name: string, parameters: NumericExpression[]) {
    this.name = name;
    this.parameters = parameters;
  }

  evaluate(state: State): NumberValue {
    let parameters = this.parameters.map(e => e.evaluate(state));
    let val = state.getValue(this.name, this.parameters.length);
    if (val instanceof NumberArray) {
      let arr = val as NumberArray;
      return arr.leftApply(state, parameters);
    }
    return val as NumberValue;
  }
}

/// 数字常量 在这里确定数字具体类型
export class ConstNumberExpression implements NumericExpression {
  value: number;

  constructor(value: number) {
    this.value = value;
  }

  evaluate(state: State): number {
    return this.value;
  }
}

/// 乘法表达式
class NumberMulExpression implements NumericExpression {
  left: NumericExpression;

  right: NumericExpression;

  evaluate(state: State): number {
    let lf = this.left.evaluate(state);
    let rg = this.right.evaluate(state);
    return lf * rg;
  }

  constructor(left: NumericExpression, right: NumericExpression) {
    this.left = left;
    this.right = right;
  }
}

/// 除法表达式
class NumberDivExpression implements NumericExpression {
  left: NumericExpression;

  right: NumericExpression;

  evaluate(state: State): number {
    let lf = this.left.evaluate(state);
    let rg = this.right.evaluate(state);
    return lf / rg;
  }

  constructor(left: NumericExpression, right: NumericExpression) {
    this.left = left;
    this.right = right;
  }
}

class StringConstExpression implements StringExpression {
  value: string;

  constructor(value: string) {
    this.value = value;
  }

  evaluate(state: State): string {
    return this.value;
  }
}

class StringVarExpression implements StringExpression {
  name: string;

  constructor(name: string) {
    this.name = name;
  }

  evaluate(state: State): string {
    let value = state.stringValues.get(this.name);
    if (value !== null) {
      return value as string;
    } else {
      state.abort(`Could not find string variable ${this.name}`);
      return '';
    }
  }
}

/// 关系表达式
class RelationalExpression implements Expression {
  identify: String;

  left: Expression;

  right: Expression;

  constructor(identify: string, left: Expression, right: Expression) {
    this.identify = identify;
    this.left = left;
    this.right = right;
  }

  evaluate(state: State): boolean {
    if ((this.left as NumericExpression).evaluate) {
      let lf = this.left as NumericExpression;
      let rg = this.right as NumericExpression;
      let lfr = lf.evaluate(state);
      let rgr = rg.evaluate(state);
      switch (this.identify) {
        case '=':
          return lfr === rgr;
        case '<>':
          return lfr !== rgr;
        case '<':
          return lfr < rgr;
        case '>':
          return lfr > rgr;
        case '<=':
          return lfr <= rgr;
        case '>=':
          return lfr >= rgr;
        default:
          return false;
      }
    } else {
      let lf = this.right as StringExpression;
      let rg = this.right as StringExpression;
      let lfs = lf.evaluate(state);
      let rgs = rg.evaluate(state);
      switch (this.identify) {
        case '=':
          return lfs === rgs;
        case '<>':
          return lfs !== rgs;
        default:
          return false;
      }
    }
  }
}

class Parser {
  program: Program;

  lexer: Lexer;

  pushBackBuffer: Token[] = [];

  constructor(lexer: Lexer) {
    this.lexer = lexer;
    this.program = new Program();
  }

  nextToken(): Token {
    if (this.pushBackBuffer.length > 0) {
      return this.pushBackBuffer.pop()!;
    }
    let result = this.lexer.next();
    if (!result.done) {
      return result.value;
    } else {
      return new Token('endOfFile', '<end of file>', undefined, undefined);
    }
  }

  pushToken(token: Token): void {
    this.pushBackBuffer.push(token);
  }

  peekToken(): Token {
    let result = this.nextToken();
    this.pushToken(result);
    return result;
  }

  consumeKind(kind: string): Token {
    let token = this.nextToken();
    if (token.kind !== kind) {
      throw new Error(`kAt ${token.sourceLineNumber ?? -1}: expected ${kind} but got: ${token.string}`);
    }
    return token;
  }

  consumeToken(string: string): Token {
    let token = this.nextToken();
    if (token.string.toLowerCase() !== string.toLowerCase()) {
      throw new Error(`At ${token.sourceLineNumber ?? -1}: expected ${string} but got: ${token.string}`);
    }
    return token;
  }

  parseVariable(): Variable {
    let name = this.consumeKind('identifier').string;
    let result = new Variable(name, []);
    if (this.peekToken().string === '(') {
      do {
        this.nextToken();
        result.parameters.push(this.parseNumericExpression());
      } while (this.peekToken().string === ',');
      this.consumeToken(')');
    }
    return result;
  }

  parseNumericExpression(): NumericExpression {
    let negate = false;
    switch (this.peekToken().string) {
      case '+':
        this.nextToken();
        break;
      case '-':
        negate = true;
        this.nextToken();
        break;
      default:
        break;
    }
    let term: NumericExpression = this.parseTerm();
    if (negate) {
      term = new NumberNegExpression(term);
    }
    let ok = true;
    while (ok) {
      switch (this.peekToken().string) {
        case '+':
          this.nextToken();
          term = new NumberAddExpression(term, this.parseTerm());
          break;
        case '-':
          this.nextToken();
          term = new NumberSubExpression(term, this.parseTerm());
          break;
        default:
          ok = false;
          break;
      }
    }
    return term;
  }

  /// Primary => identifier(numberExpression,numberExpression...) | number | (numberExpression)
  parsePrimary(): NumericExpression {
    let token = this.nextToken();
    switch (token.kind) {
      case 'identifier':
        let result = new PrimaryExpression(token.string, []);
        if (this.peekToken().string === '(') {
          do {
            this.nextToken();
            result.parameters.push(this.parseNumericExpression());
          } while (this.peekToken().string === ',');
          this.consumeToken(')');
        }

        return result;
      case 'number':
        return new ConstNumberExpression(token.value as number);
      case 'operator':
        switch (token.string) {
          case '(':
            let result = this.parseNumericExpression();
            this.consumeToken(')');
            return result;
          default:
            break;
        }
        break;
      default:
        break;
    }
    throw new Error(`At ${token.sourceLineNumber}: expected identifier, number, or (, but got: ${token.string}`);
  }

  parseFactor(): NumericExpression {
    let primary: NumericExpression = this.parsePrimary();
    let ok = true;
    while (ok) {
      switch (this.peekToken().string) {
        case '^':
          this.nextToken();
          primary = new FactorExpression(primary, this.parsePrimary());
          break;
        default:
          ok = false;
          break;
      }
    }
    return primary;
  }

  parseTerm(): NumericExpression {
    let factor: NumericExpression = this.parseFactor();
    let ok = true;
    while (ok) {
      switch (this.peekToken().string) {
        case '*':
          this.nextToken();
          factor = new NumberMulExpression(factor, this.parseFactor());
          break;
        case '/':
          this.nextToken();
          factor = new NumberDivExpression(factor, this.parseFactor());
          break;
        default:
          ok = false;
          break;
      }
    }
    return factor;
  }

  parseStringExpression(): StringExpression {
    let token = this.nextToken();
    switch (token.kind) {
      case 'string':
        return new StringConstExpression(token.value as string);
      case 'identifier':
        this.consumeToken('$');
        return new StringVarExpression(token.string);
      default:
        throw new Error(`At ${token.sourceLineNumber}: expected string expression but got ${token.string}`);
    }
  }

  isStringExpression(): boolean {
    let token = this.nextToken();
    if (token.kind === 'string') {
      this.pushToken(token);
      return true;
    }
    if (token.kind === 'identifier') {
      let result = this.peekToken().string === '$';
      this.pushToken(token);
      return result;
    }
    this.pushToken(token);
    return false;
  }

  parseRelationalExpress(): RelationalExpression {
    if (this.isStringExpression()) {
      let left = this.parseStringExpression();
      let op = this.nextToken();
      return new RelationalExpression(op.string, left, this.parseStringExpression());
    }
    let left = this.parseNumericExpression();
    let op = this.nextToken();
    return new RelationalExpression(op.string, left, this.parseNumericExpression());
  }

  parseNonNegativeInteger(): number {
    let token = this.nextToken();
    if (!/^[0-9]+$/.test(token.string)) {
      throw new Error(`At ${token.sourceLineNumber}: expected a line number but got: ${token.string}`);
    }
    return token.value as number;
  }

  parseStatement(): Statement {
    let statement: Statement;
    let lineNumber = this.consumeKind('userLineNumber').userLineNumber!;
    let command = this.nextToken();
    let sourceLineNumber = command.sourceLineNumber!;
    switch (command.kind) {
      case 'keyword':
        switch (command.string.toLowerCase()) {
          case 'let':
            let letVal = this.parseVariable();
            this.consumeToken('=');
            statement = new LetStatement(lineNumber, sourceLineNumber, this.parseNumericExpression(), letVal);
            break;
          case 'if':
            let condition = this.parseRelationalExpress();
            this.consumeToken('then');
            statement = new IfStatement(lineNumber, sourceLineNumber, this.parseNonNegativeInteger(), condition);
            break;
          case 'for':
            return this.parseForStatements(lineNumber, sourceLineNumber, statement!);
          case 'next':
            statement = new NextStatement(lineNumber, sourceLineNumber, this.consumeKind('identifier').string);
            break;
          case 'print':
            statement = this.parsePrintStatements(lineNumber, sourceLineNumber);
            break;
          case 'dim': {
            statement = this.parseDimStatement(lineNumber, sourceLineNumber);
            break;
          }
          case 'end':
            statement = new EndStatement(lineNumber, sourceLineNumber);
            break;
          default:
            throw new Error(`At ${command.sourceLineNumber}: unexpected command but got: ${command.string}`);
        }
        break;
      case 'remark':
        break;
      default:
        throw new Error(`At ${command.sourceLineNumber}: expected command but got: ${command.string} (of kind ${command.kind})`);
    }
    this.consumeKind('newLine');
    this.program.statements.set(statement!.lineNumber, statement!);
    return statement!;
  }

  parseForStatements(lineNumber: number, sourceLineNumber: number, statement: Statement): Statement {
    let forVal = this.consumeKind('identifier').string;
    this.consumeToken('=');
    let initial = this.parseNumericExpression();
    this.consumeToken('to');
    let limit = this.parseNumericExpression();
    let step: number | NumericExpression;
    if (this.peekToken().string === 'step') {
      this.nextToken();
      step = this.parseNumericExpression();
    } else {
      step = 1;
    }
    statement = new ForStatement(lineNumber, sourceLineNumber, forVal, initial, limit, step);
    this.consumeKind('newLine');
    let lastStatement = this.parseStatements();
    if (lastStatement instanceof NextStatement) {
      if (lastStatement.variable !== (statement as ForStatement).variable) {
        throw new Error(`At ${lastStatement.sourceLineNumber}: expected next for \((statement as! ForStatement).variable) but got ${lastStatement.variable})`);
      } else {
        lastStatement.target = statement as ForStatement;
        (statement as ForStatement).target = lastStatement;
        this.program.statements.set(lineNumber, statement);
        return statement;
      }
    } else {
      throw new Error('At (lastStatement.sourceLineNumber): expected next statement');
    }
  }

  parsePrintStatements(lineNumber: number, sourceLineNumber: number): Statement {
    let statement = new PrintStatement(lineNumber, sourceLineNumber);
    let pok = true;
    while (pok) {
      switch (this.peekToken().string) {
        case ',':
          this.nextToken();
          statement.items.push(['comma', null]);
          break;
        case ';':
          this.nextToken();
          break;
        case 'tab':
          this.nextToken();
          this.consumeToken('(');
          statement.items.push(['tab', this.parseNumericExpression()]);
          break;
        case '\n':
          pok = false;
          break;
        default:
          if (this.isStringExpression()) {
            statement.items.push(['string', this.parseStringExpression()]);
            break;
          } else {
            statement.items.push(['number', this.parseNumericExpression()]);
            break;
          }
      }
    }
    return statement;
  }

  parseDimStatement(lineNumber: number, sourceLineNumber: number): DimStatement {
    let statement = new DimStatement(lineNumber, sourceLineNumber);
    while (true) {
      let name = this.consumeKind('identifier').string;
      this.consumeToken('(');
      let bounds = new Array<number>();
      bounds.push(this.parseNonNegativeInteger());
      if (this.peekToken().string === ',') {
        this.nextToken();
        bounds.push(this.parseNonNegativeInteger());
      }
      this.consumeToken(')');
      statement.items.push([name, bounds]);
      if (this.peekToken().string !== ',') {
        break;
      }
      this.consumeToken(',');
    }
    return statement;
  }

  parseStatements(): Statement {
    let statementRes: Statement;
    do {
      statementRes = this.parseStatement();
    } while (!(statementRes instanceof EndStatement) && !(statementRes instanceof NextStatement));
    return statementRes;
  }

  parse(): Program {
    let lastStatement = this.parseStatements();
    if (!(lastStatement instanceof EndStatement)) {
      throw new Error(`At ${lastStatement.sourceLineNumber}: expected end`);
    }
    return this.program;
  }
}

class Program {
  base = 0;

  statements: Map<number, Statement>;

  constructor() {
    this.statements = new Map();
  }

  process(state: State): void {
    state.validate(state.program === this, 'State must match program');
    let maxLineNumber = Math.max(...this.statements.keys());
    while (state.nextLineNumber !== undefined) {
      state.validate(state.nextLineNumber <= maxLineNumber, 'Went out of bounds of the program');
      let statement = this.statements.get(state.nextLineNumber);
      state.nextLineNumber += 1;
      if (!statement) {
        continue;
      }
      state.statement = statement;
      if (statement instanceof PrintStatement) {
        let output = statement.process(state);
        if (DEBUG) {
          print(output[1]);
        }
      } else if ((statement as VoidStatement).process) {
        let voidState = statement as VoidStatement;
        voidState.process(state);
      } else {
        continue;
      }
    }
  }
}

export { Parser, State };
