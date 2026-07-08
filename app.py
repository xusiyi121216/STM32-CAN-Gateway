import serial
import threading
from flask import Flask, render_template
from flask_socketio import SocketIO
from datetime import datetime

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

ser = None
monitoring = False

def parse_line(line):
    data = {}
    try:
        if 'TX 0x101' in line:
            frame_num = int(line.split('[')[1].split(']')[0])
            temp_str = line.split('Temp=')[1].split(' ')[0]
            data = {'type': 'temp', 'frame': frame_num, 'value': float(temp_str)}
        elif 'TX 0x102' in line:
            frame_num = int(line.split('[')[1].split(']')[0])
            volt_str = line.split('Volt=')[1].split(' ')[0]
            data = {'type': 'volt', 'frame': frame_num, 'value': float(volt_str)}
        elif 'TEC=' in line:
            tec = int(line.split('TEC=')[1].split(' ')[0])
            rec = int(line.split('REC=')[1].split(' ')[0])
            data = {'type': 'status', 'tec': tec, 'rec': rec}
        elif 'Bus-Off' in line:
            data = {'type': 'busoff'}
        elif 'Recovered' in line:
            data = {'type': 'recovered'}
    except:
        pass
    return data

def serial_reader():
    global ser, monitoring
    while monitoring:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                data = parse_line(line)
                if data:
                    data['timestamp'] = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                    socketio.emit('can_data', data)
        except:
            break

@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('connect')
def on_connect():
    global ser, monitoring
    try:
        ser = serial.Serial('COM6', 115200, timeout=1)
        monitoring = True
        t = threading.Thread(target=serial_reader, daemon=True)
        t.start()
        print("Serial connected")
    except Exception as e:
        print(f"Serial error: {e}")

@socketio.on('disconnect')
def on_disconnect():
    global monitoring
    monitoring = False
    if ser:
        ser.close()

if __name__ == '__main__':
    socketio.run(app, debug=False, port=5000)