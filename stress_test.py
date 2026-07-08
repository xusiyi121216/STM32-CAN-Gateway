import serial
import time
import threading
from datetime import datetime

PORT = 'COM6'
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=1)

results = {
    'total_frames': 0,
    'lost_frames': 0,
    'bus_off_count': 0,
    'error_count': 0,
    'max_tec': 0,
    'start_time': None,
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
                        if results['start_time'] is None:
                            results['start_time'] = time.time()
                        results['total_frames'] += 1
                        if expected_frame is not None and frame_num != expected_frame:
                            lost = frame_num - expected_frame
                            results['lost_frames'] += lost
                            print(f"  ⚠️  丢帧：expected={expected_frame} got={frame_num} lost={lost}")
                        expected_frame = frame_num + 1
                    except:
                        pass
                elif 'TEC=' in line:
                    try:
                        tec = int(line.split('TEC=')[1].split(' ')[0])
                        if tec > 0:
                            results['error_count'] += 1
                            results['max_tec'] = max(results['max_tec'], tec)
                    except:
                        pass
                elif 'Bus-Off' in line:
                    results['bus_off_count'] += 1
        except:
            break

def print_stress_report(test_name, duration):
    with lock:
        total = results['total_frames']
        lost = results['lost_frames']
        rate = lost / (total + lost) * 100 if (total + lost) > 0 else 0
        elapsed = time.time() - results['start_time'] if results['start_time'] else duration
        fps = total / elapsed if elapsed > 0 else 0
        print(f"\n{'='*55}")
        print(f"  压力测试报告：{test_name}")
        print(f"{'='*55}")
        print(f"  持续时间：     {duration}秒")
        print(f"  总帧数：       {total}")
        print(f"  丢帧数：       {lost}")
        print(f"  丢帧率：       {rate:.2f}%")
        print(f"  实际帧率：     {fps:.1f} fps")
        print(f"  Bus-Off次数：  {results['bus_off_count']}")
        print(f"  最大TEC：      {results['max_tec']}")
        print(f"{'='*55}")
        result = "✅ PASS" if rate < 1.0 and results['bus_off_count'] == 0 else "❌ FAIL"
        print(f"  结果：{result}（丢帧率<1%且无Bus-Off为PASS）")
        print(f"{'='*55}\n")

print("="*55)
print("  CAN Bus 压力测试")
print(f"  端口：{PORT}  波特率：{BAUD}")
print(f"  开始时间：{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
print("="*55)

t = threading.Thread(target=monitor_thread, daemon=True)
t.start()

# 场景一：10ms高频发送，持续30秒
print("\n[STRESS 1] 10ms高频发送压力测试（30秒）")
print("  → 板子已烧录10ms延迟版本，保持所有线路连接")
results['start_time'] = None
results['total_frames'] = 0
results['lost_frames'] = 0
results['bus_off_count'] = 0
results['error_count'] = 0
results['max_tec'] = 0
expected_frame = None
time.sleep(30)
print_stress_report("10ms高频发送", 30)

monitoring = False
ser.close()
print(f"结束时间：{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")