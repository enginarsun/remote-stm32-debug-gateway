from flask import Flask, render_template_string, jsonify
import subprocess
import serial

app = Flask(__name__)

HTML = """
<!DOCTYPE html>
<html>
<head>
    <title>Remote STM32 Debug Gateway</title>
    <style>
        body { font-family: monospace; background: #1a1a1a; color: #00ff00; padding: 20px; }
        button { background: #00ff00; color: #000; border: none; padding: 15px 30px; font-size: 18px; cursor: pointer; border-radius: 5px; margin-right: 10px; }
        button:hover { background: #00cc00; }
        .box { background: #000; border: 1px solid #00ff00; padding: 15px; margin-top: 20px; white-space: pre-wrap; min-height: 150px; max-height: 300px; overflow-y: auto; }
        h2 { margin-top: 30px; }
    </style>
</head>
<body>
    <h1>Remote STM32 Debug Gateway</h1>
    <button onclick="flashIt()">Flash Et</button>

    <h2>Flash Output</h2>
    <div id="output" class="box">Hazir. Flash Et butonuna bas.</div>

    <h2>UART Log (canli)</h2>
    <div id="uart" class="box">Bekleniyor...</div>

    <script>
        function flashIt() {
            document.getElementById('output').innerText = 'Flashing...';
            fetch('/flash')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('output').innerText = data.output;
                });
        }

        function updateUart() {
            fetch('/uart')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('uart').innerText = data.lines;
                });
        }

        setInterval(updateUart, 1000);
        updateUart();
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML)

@app.route('/flash')
def flash():
    result = subprocess.run(
        ['st-flash', '--connect-under-reset', 'write', '/home/engin/blink/blink.bin', '0x08000000'],
        capture_output=True, text=True
    )
    output = result.stdout + result.stderr
    return jsonify({'output': output})

@app.route('/uart')
def uart():
    try:
        ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1, dsrdtr=False, rtscts=False)
        lines = []
        for _ in range(10):
            line = ser.readline().decode(errors='ignore').strip()
            if line:
                lines.append(line)
        ser.close()
        return jsonify({'lines': '\n'.join(lines) if lines else 'Veri bekleniyor...'})
    except Exception as e:
        return jsonify({'lines': 'UART hatasi: ' + str(e)})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
