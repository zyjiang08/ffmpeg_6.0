#!/bin/bash

# 频道切换功能测试脚本
# 用法：./test_channel_switch.sh

echo "=========================================="
echo "  FFplay 频道切换功能测试"
echo "=========================================="
echo ""

# 检查 ffplay 是否存在
if [ ! -f "./ffplay" ]; then
    echo "❌ 错误：找不到 ffplay 可执行文件"
    echo "请先运行: make ffplay -j8"
    exit 1
fi

echo "✅ ffplay 可执行文件存在"
echo ""

# 检查 H3 协议支持
if ./ffplay -protocols 2>&1 | grep -q "^ *h3$"; then
    echo "✅ H3 协议支持已启用"
else
    echo "❌ 警告：H3 协议可能未启用"
fi
echo ""

echo "=========================================="
echo "  使用说明"
echo "=========================================="
echo ""
echo "默认配置的两个频道："
echo "  频道 1: h3://120.79.21.28:8443/600k_cbr.flv"
echo "  频道 2: h3://120.79.21.28:8443/800k_cbr.flv"
echo ""
echo "界面按钮（从左到右）："
echo "  [▶] Play/Pause  - 播放/暂停"
echo "  [■] Stop        - 停止退出"
echo "  [⏪] <<          - 后退10秒"
echo "  [⏩] >>          - 前进10秒"
echo "  [CH] 频道切换    - 点击切换到下一个频道"
echo "  [⟳] 压力测试    - 点击开启/关闭自动切换（每3秒）"
echo ""
echo "=========================================="
echo "  测试步骤"
echo "=========================================="
echo ""
echo "测试 1: 手动切换"
echo "  1. 等待视频开始播放"
echo "  2. 点击 'CH' 按钮"
echo "  3. 观察是否切换到另一个频道"
echo "  4. 再次点击 'CH' 切换回来"
echo "  5. 重复几次确认功能正常"
echo ""
echo "测试 2: 自动压力测试"
echo "  1. 点击 'Test' 按钮（会变成红色）"
echo "  2. 观察播放器每3秒自动切换"
echo "  3. 观察控制台输出的切换日志"
echo "  4. 再次点击 'Test' 停止测试"
echo ""
echo "退出: 按 Q 或 ESC 键"
echo ""
echo "=========================================="
echo ""

read -p "按 Enter 键启动 ffplay..."
echo ""

echo "🚀 启动 FFplay..."
echo ""

# 启动 ffplay
./ffplay h3://120.79.21.28:8443/600k_cbr.flv

echo ""
echo "=========================================="
echo "  测试结束"
echo "=========================================="
