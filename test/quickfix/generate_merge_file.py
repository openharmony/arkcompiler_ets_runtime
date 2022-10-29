#!/usr/bin/env python3
#coding: utf-8
"""
Copyright (c) 2022-2023 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: run script
    input: input file
    output: output file
    prefix: prefix
"""

import argparse
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, required=True)
    parser.add_argument('--output', type=str, required=True)
    parser.add_argument('--prefix', type=str, required=True)

    args = parser.parse_args()

    with open(args.input, 'r') as input_file, open(args.output, 'w') as output_file:
        for line in input_file:
            output_line = args.prefix + line
            output_file.write(output_line)

if __name__ == '__main__':
    sys.exit(main())
