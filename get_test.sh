#!/bin/bash

# 检查参数数量
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <lv> <case>"
    exit 1
fi

# 获取参数
lv=$1
case=$2

# 构建文件路径前缀
file_prefix="/opt/bin/testcases/lv${lv}/${case}"

# 查找匹配的文件
file_path=$(find "/opt/bin/testcases/lv${lv}/" -type f -name "${case}*.c" | head -n 1)

# 检查是否找到文件
if [ -z "$file_path" ]; then
    echo "File not found with prefix: $file_prefix and extension: .c"
    exit 1
fi

# 输出文件内容
cat "$file_path"
