import serial
import time
from datetime import datetime

ser = serial.Serial('COM6', 115200, timeout=1)

print("CAN Fault Monitor Started")
print("-" * 60)

expected_frame = None
total_frames = 0
lost_frames = 0
bus_off_count = 0
error_count = 0

try:
    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not line:
            continue

        timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]

        # 解析TX 0x101帧，用帧号做丢帧检测
        if 'TX 0x101' in line:
            try:
                frame_num = int(line.split('[')[1].split(']')[0])
                total_frames += 1

                if expected_frame is not None and frame_num != expected_frame:
                    lost = frame_num - expected_frame
                    lost_frames += lost
                    print(f"[{timestamp}] ⚠️  LOST {lost} frames! expected={expected_frame} got={frame_num}")

                expected_frame = frame_num + 1
                print(f"[{timestamp}] {line}")
            except:
                pass

        elif 'TX 0x102' in line:
            print(f"[{timestamp}] {line}")

        elif 'TX 0x103' in line:
            print(f"[{timestamp}] {line}")

        # TEC/REC错误计数器
        elif 'TEC=' in line:
            try:
                tec = int(line.split('TEC=')[1].split(' ')[0])
                rec = int(line.split('REC=')[1].split(' ')[0])
                if tec > 0 or rec > 0:
                    error_count += 1
                    print(f"[{timestamp}] ⚠️  ERROR #{error_count}: TEC={tec} REC={rec}")
                else:
                    print(f"[{timestamp}] STATUS: {line}")
            except:
                pass

        # Bus-Off检测
        elif 'Bus-Off' in line:
            bus_off_count += 1
            print(f"[{timestamp}] 🚨 Bus-Off #{bus_off_count}")

        elif 'Recovered' in line:
            print(f"[{timestamp}] ✅ Recovered")

except KeyboardInterrupt:
    print(f"\n{'='*60}")
    print(f"监控报告：")
    print(f"  总帧数：{total_frames}")
    print(f"  丢帧数：{lost_frames}")
    print(f"  丢帧率：{lost_frames/total_frames*100:.1f}%" if total_frames > 0 else "  丢帧率：N/A")
    print(f"  Bus-Off次数：{bus_off_count}")
    print(f"  错误计数器告警次数：{error_count}")
    ser.close()