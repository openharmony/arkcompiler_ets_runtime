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
import json


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--enum',
                        type=str,
                        required=True,
                        help='type_enum.json file path')
    parser.add_argument('--metadata',
                        type=str,
                        required=True,
                        help='metadata folder path')
    parser.add_argument('--output',
                        type=str,
                        required=True,
                        help='generated metadata.json file path')
    parser.add_argument('--version',
                        type=str,
                        required=True,
                        help='version number')
    args = parser.parse_args()
    return args


def parse_type_enums(file_path):
    with open(file_path, 'r') as f:
        type_enums = json.load(f)
    return type_enums["type_enum"]


def parse_type_list(dir_path):
    type_list, type_layout = [], {}
    for file_name in os.listdir(dir_path):
        if not os.path.isfile(os.path.join(dir_path, file_name)) or not _is_metadata_file(file_name):
            continue
        with open(os.path.join(dir_path, file_name), 'r') as f:
            metadata = json.load(f)
            if file_name.endswith("_layout.json"):
                type_layout["Dictionary_layout"] = metadata
            elif file_name == "type_range.json":
                type_layout["Type_range"] = metadata
            else:
                type_list.append(metadata)
    return type_list, type_layout


def _is_metadata_file(file_name):
    return file_name.endswith(".json") and file_name != "type_enums.json"


def generate_metadata_json(enum_list, metadata_list, type_layout, file_path, version):
    metadata_dict = {"type_enum": enum_list, "type_list": [], "type_layout": type_layout, "version": version}
    for metadata in metadata_list:
        metadata_dict["type_list"].append(metadata)
    with os.fdopen(os.open(file_path, os.O_RDWR | os.O_CREAT | os.O_TRUNC, 0o777), 'w+') as f:
        json_str = json.dumps(metadata_dict)
        f.write(json_str)


if __name__ == '__main__':
    arguments = parse_args()
    type_enum_list = parse_type_enums(arguments.enum)
    type_metadata_list, type_layout_list = parse_type_list(arguments.metadata)
    generate_metadata_json(type_enum_list, type_metadata_list, type_layout_list, arguments.output, arguments.version)
