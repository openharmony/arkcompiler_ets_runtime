#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Copyright (c) 2024 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: metadata.json generate
"""
import argparse
import os
from shutil import copyfile


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--metadata',
                        type=str,
                        required=True,
                        help='metadata folder path')
    parser.add_argument('--outputs',
                        type=str,
                        required=True,
                        help='copy destination')
    args = parser.parse_args()
    return args


def copy_metadata(input_path, output_path):
    if not os.path.exists(output_path):
        os.makedirs(output_path, 0o777, True)
    for file_name in os.listdir(input_path):
        if os.path.isfile(os.path.join(input_path, file_name)) and file_name.endswith('.json'):
            copyfile(os.path.join(input_path, file_name), os.path.join(output_path, file_name))


if __name__ == '__main__':
    arguments = parse_args()
    copy_metadata(arguments.metadata, arguments.outputs)
