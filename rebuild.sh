#!/bin/bash

# FFplay 快速增量编译脚本
# 用法: ./rebuild.sh [clean]

echo "=========================================="
echo "  FFplay 增量编译"
echo "=========================================="
echo ""

# 检查是否需要 clean
if [ "$1" = "clean" ]; then
    echo "🧹 清理旧的编译产物..."
    make clean
    echo "✅ 清理完成"
    echo ""
fi

# 记录开始时间
start_time=$(date +%s)

echo "🔨 开始编译 ffplay..."
echo ""

# 执行编译
if make ffplay -j8 2>&1 | tee /tmp/ffplay_rebuild.log; then
    # 记录结束时间
    end_time=$(date +%s)
    duration=$((end_time - start_time))

    echo ""
    echo "=========================================="
    echo "  ✅ 编译成功！"
    echo "=========================================="
    echo ""
    echo "⏱️  编译耗时: ${duration} 秒"
    echo ""

    # 显示文件信息
    if [ -f "./ffplay" ]; then
        echo "📦 可执行文件信息:"
        ls -lh ffplay
        echo ""

        # 检查文件大小
        size=$(stat -f%z ffplay 2>/dev/null || stat -c%s ffplay 2>/dev/null)
        size_mb=$((size / 1024 / 1024))
        echo "📊 文件大小: ${size_mb} MB"
        echo ""

        # 检查 H3 协议支持
        echo "🔍 检查 H3 协议支持..."
        if ./ffplay -protocols 2>&1 | grep -q "^ *h3$"; then
            echo "✅ H3 协议支持已启用"
        else
            echo "⚠️  警告: H3 协议未启用"
        fi
        echo ""

        # 显示编译摘要
        echo "📋 编译摘要:"
        grep -E "CC|LD|STRIP" /tmp/ffplay_rebuild.log | tail -10
        echo ""

        echo "=========================================="
        echo ""

        # 询问是否运行测试
        read -p "是否立即测试 ffplay？(y/n) " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo ""
            echo "🚀 启动 ffplay 测试..."
            echo ""
            if [ -f "./test_channel_switch.sh" ]; then
                ./test_channel_switch.sh
            else
                ./ffplay h3://120.79.21.28:8443/600k_cbr.flv
            fi
        fi
    else
        echo "⚠️  警告: ffplay 可执行文件不存在"
        echo ""
    fi
else
    # 记录结束时间
    end_time=$(date +%s)
    duration=$((end_time - start_time))

    echo ""
    echo "=========================================="
    echo "  ❌ 编译失败！"
    echo "=========================================="
    echo ""
    echo "⏱️  失败前耗时: ${duration} 秒"
    echo ""
    echo "📋 错误日志 (最后20行):"
    echo "----------------------------------------"
    tail -20 /tmp/ffplay_rebuild.log
    echo "----------------------------------------"
    echo ""
    echo "完整日志: /tmp/ffplay_rebuild.log"
    echo ""
    exit 1
fi
