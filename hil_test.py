import serial
import time
import threading
from datetime import datetime

# ===== 配置 =====
PORT = 'COM6'
BAUD = 115200
TEST_DURATION = 10  # 每个测试场景持续秒数

ser = serial.Serial(PORT, BAUD, timeout=1)

# ===== 数据收集 =====
results = {
    'total_frames': 0,
    'lost_frames': 0,
    'bus_off_count': 0,
    'error_count': 0,
    'max_tec': 0,
    'max_rec': 0,
}
expected_frame = None
monitoring = True
lock = threading.Lock()

def monitor_thread():
    global expected_frame, monitoring
    while monitoring:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                continue
            with lock:
                if 'TX 0x101' in line:
                    try:
                        frame_num = int(line.split('[')[1].split(']')[0])
                        results['total_frames'] += 1
                        if expected_frame is not None and frame_num != expected_frame:
                            lost = frame_num - expected_frame
                            results['lost_frames'] += lost
                        expected_frame = frame_num + 1
                    except:
                        pass
                elif 'TEC=' in line:
                    try:
                        tec = int(line.split('TEC=')[1].split(' ')[0])
                        rec = int(line.split('REC=')[1].split(' ')[0])
                        if tec > 0 or rec > 0:
                            results['error_count'] += 1
                            results['max_tec'] = max(results['max_tec'], tec)
                            results['max_rec'] = max(results['max_rec'], rec)
                    except:
                        pass
                elif 'Bus-Off' in line:
                    results['bus_off_count'] += 1
        except:
            break

def reset_results():
    global expected_frame
    with lock:
        for k in results:
            results[k] = 0
        expected_frame = None

def print_report(test_name, duration, expect_busoff=False):
    with lock:
        total = results['total_frames']
        lost = results['lost_frames']
        rate = lost / (total + lost) * 100 if (total + lost) > 0 else 0
        print(f"\n{'='*55}")
        print(f"  测试报告：{test_name}")
        print(f"{'='*55}")
        print(f"  持续时间：     {duration}秒")
        print(f"  总帧数：       {total}")
        print(f"  丢帧数：       {lost}")
        print(f"  丢帧率：       {rate:.1f}%")
        print(f"  Bus-Off次数：  {results['bus_off_count']}")
        print(f"  错误告警次数： {results['error_count']}")
        print(f"  最大TEC：      {results['max_tec']}")
        print(f"  最大REC：      {results['max_rec']}")
        print(f"{'='*55}")
        if expect_busoff:
            result = "✅ PASS" if results['bus_off_count'] > 0 and lost == 0 else "❌ FAIL"
        else:
            result = "✅ PASS" if lost == 0 and results['bus_off_count'] == 0 else "❌ FAIL"
        print(f"  结果：{result}")
        print(f"{'='*55}\n")

# ===== 测试场景 =====

def test_normal(duration=TEST_DURATION):
    print(f"\n[TEST 1] 正常通信稳定性测试 ({duration}秒)")
    print("  → 保持所有线路连接，观察稳定性")
    reset_results()
    time.sleep(duration)
    print_report("正常通信稳定性", duration)

def test_disconnect(duration=TEST_DURATION):
    print(f"\n[TEST 2] 断线故障注入测试 ({duration}秒)")
    print("  → 请在3秒后手动拔掉CANL线，测试结束前重新接上")
    reset_results()
    time.sleep(3)
    print("  ⚡ 现在拔掉CANL线！")
    time.sleep(duration - 6)
    print("  ⚡ 现在重新接上CANL线！")
    time.sleep(3)
    print_report("断线故障注入", duration, expect_busoff=True)

def test_recovery(duration=8):
    print(f"\n[TEST 3] 自动恢复验证测试 ({duration}秒)")
    print("  → 拔线3秒后重接，验证系统自动恢复")
    reset_results()
    time.sleep(2)
    print("  ⚡ 请拔掉CANL线！")
    time.sleep(3)
    print("  ⚡ 请重新接上CANL线！")
    time.sleep(3)
    print_report("自动恢复验证", duration, expect_busoff=True)

# ===== 主程序 =====
print("="*55)
print("  CAN Bus HIL自动化测试框架")
print(f"  端口：{PORT}  波特率：{BAUD}")
print(f"  开始时间：{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
print("="*55)

t = threading.Thread(target=monitor_thread, daemon=True)
t.start()
time.sleep(1)

try:
    test_normal(duration=10)
    test_disconnect(duration=10)
    test_recovery(duration=8)

    print("\n✅ 所有测试场景完成！")
    print(f"  结束时间：{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

except KeyboardInterrupt:
    print("\n测试中断")

finally:
    monitoring = False
    ser.close()