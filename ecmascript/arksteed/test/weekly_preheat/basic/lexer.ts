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

/// 词/token，词法分析结果
class Token {
  kind: string;

  string: string;

  sourceLineNumber?: number;

  userLineNumber?: number;

  value: number | string | null = null;

  constructor(kind: string, string: string, sourceLineNumber?: number, userLineNumber?: number) {
    this.kind = kind;
    this.string = string;
    this.sourceLineNumber = sourceLineNumber;
    this.userLineNumber = userLineNumber;
  }
}

/// 词法分析器
class Lexer implements Iterable<Token>, Iterator<Token> {
  /// 源代码
  source: string;
  /// 源代码行号
  sourceLineNumber: number = 1;
  /// 是否为行首
  isLineStart: boolean;
  /// 用户输入的行号
  userLineNumber?: number;
  /// 当前解析的代码，也记录了解析的进度
  currentCode: string | null = null;
  /// 所有的代码行
  lines: string[];
  /// 标识符正则
  isIdentifier: RegExp = new RegExp('^[a-z_]([a-z0-9_]*)');
  /// 数字正则
  isNumber: RegExp = new RegExp('^(([0-9]+(\\.([0-9]*))?)|(\\.[0-9]+)(e([+-]?)([0-9]+))?)');

  /// 字符串正则
  isString: RegExp = new RegExp('^\\"([^\\"]|(\\"\\"))*\\"');

  /// 关键字正则
  isKeyword: RegExp = new RegExp(
    '^((base)|(data)|(def)|(dim)|(end)|(for)|(go)|(gosub)|(goto)|(if)|(input)|(let)|(next)|(on)|(option)|(print)|(randomize)|(read)|(restore)|(return)|(step)|(stop)|(sub)|(then)|(to))'
  );
  /// 操作符正则
  isOperator: RegExp = new RegExp('^(-|\\+|\\*|\\/|\\^|\\(|\\)|(<[>=]?)|(>=?)|=|,|\\$|;)');

  isRem: RegExp = new RegExp('^rem\\s.*');

  constructor(source: string) {
    this.source = source;
    this.lines = source.split('\n');
    this.currentCode = this.lines[this.sourceLineNumber - 1];
    this.isLineStart = true;
  }

  /// 匹配并去除空格
  consumeWhitespace(): void {
    let reg = new RegExp('^\\s+');
    let matches = this.currentCode!.match(reg);
    if (matches !== null && matches.length > 0) {
      this.currentCode = this.currentCode!.slice(matches[0].length);
    }
  }

  /// 迭代器 词法解析 provide the next token
  next(): IteratorResult<Token> {
    if (this.currentCode === null) {
      return { done: true, value: null };
    }
    if (this.isLineStart) {
      this.consumeWhitespace();
      let isnum = new RegExp('^[0-9]+');
      let matches = this.currentCode.match(isnum);
      if (matches !== null && matches.length > 0) {
        this.userLineNumber = Math.floor(Number(matches[0]));
        this.currentCode = this.currentCode.slice(matches[0].length);
        this.isLineStart = false;
        return {
          value: new Token('userLineNumber', matches[0], this.sourceLineNumber, this.userLineNumber!),
          done: false
        };
      }
    } else {
      this.consumeWhitespace();
      if (this.currentCode!.length === 0) {
        this.isLineStart = true;
        if (this.sourceLineNumber === this.lines.length) {
          this.currentCode = null;
          return {
            done: false,
            value: new Token('newLine', '\n', this.sourceLineNumber, this.userLineNumber)
          };
        } else {
          let curLineNumber = this.sourceLineNumber;
          this.sourceLineNumber += 1;
          this.currentCode = this.lines[this.sourceLineNumber - 1];
          return {
            done: false,
            value: new Token('newLine', '\n', curLineNumber, this.userLineNumber!)
          };
        }
      }
      return this.parseToken();
    }
    return { value: null, done: true };
  }

  parseToken(): IteratorResult<Token> {
    let matches = this.currentCode!.match(this.isKeyword);
    if (matches !== null && matches.length > 0) {
      this.currentCode = this.currentCode!.slice(matches[0].length);
      return { done: false, value: new Token('keyword', matches[0], this.sourceLineNumber, this.userLineNumber) };
    }
    matches = this.currentCode!.match(this.isIdentifier);
    if (matches !== null && matches.length > 0) {
      this.currentCode = this.currentCode!.slice(matches[0].length);
      return {
        done: false,
        value: new Token('identifier', matches[0], this.sourceLineNumber, this.userLineNumber!)
      };
    }
    matches = this.currentCode!.match(this.isNumber);
    if (matches !== null && matches.length > 0) {
      this.currentCode = this.currentCode!.slice(matches[0].length);
      let token = new Token('number', matches[0], this.sourceLineNumber, this.userLineNumber!);
      token.value = Number(matches[0]);
      return { done: false, value: token };
    }
    matches = this.currentCode!.match(this.isString);
    if (matches !== null && matches.length > 0) {
      this.currentCode = this.currentCode!.slice(matches[0].length);
      let str = matches[0];
      str = str.slice(1, str.length - 1);
      let strToken = new Token('string', str, this.sourceLineNumber, this.userLineNumber!);
      strToken.value = str;
      return { done: false, value: strToken };
    }
    matches = this.currentCode!.match(this.isOperator);
    if (matches !== null && matches.length > 0) {
      this.currentCode = this.currentCode!.slice(matches[0].length);
      return {
        done: false,
        value: new Token('operator', matches[0], this.sourceLineNumber, this.userLineNumber!)
      };
    }
    matches = this.currentCode!.match(this.isRem);
    if (matches !== null && matches.length > 0) {
      this.currentCode = this.currentCode!.slice(matches[0].length);
      return {
        done: false,
        value: new Token('remark', matches[0], this.sourceLineNumber, this.userLineNumber!)
      };
    }
    return { done: false, value: new Token('Error', 'Error', -1, -1) };
  }

  [Symbol.iterator](): Iterator<Token> {
    return this;
  }
}

export { Lexer, Token };
