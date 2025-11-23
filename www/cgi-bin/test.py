#!/usr/bin/env python3
"""
🚀 SUPER CGI WOW TEST 🚀
Dynamic HTML generator with real-time system info, animations, and interactive features
"""

import os
import sys
import time
import socket
import json
import random
import subprocess
from datetime import datetime

def get_system_info():
    """Fetch impressive system information"""
    try:
        uptime = subprocess.check_output(['uptime', '-p'], text=True).strip()
    except:
        uptime = "Unknown"
    
    try:
        cpu_count = subprocess.check_output(['nproc'], text=True).strip()
    except:
        cpu_count = "Unknown"
    
    try:
        memory = subprocess.check_output(['free', '-h'], text=True).split('\n')[1].split()
        mem_info = f"{memory[2]} / {memory[1]}"
    except:
        mem_info = "Unknown"
    
    return {
        'uptime': uptime,
        'cpu_count': cpu_count,
        'memory': mem_info,
        'hostname': socket.gethostname(),
        'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
    }

def generate_animation_frames():
    """Generate cute loading animation frames"""
    frames = [
        '▁▂▃▄▅▆▇█▇▆▅▄▃▂',
        '⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏',
        '◴◷◶◵',
        '⣾⣽⣻⢿⡿⣟⣯⣷',
        '🌑🌒🌓🌔🌕🌖🌗🌘'
    ]
    return random.choice(frames)

def generate_html():
    """Generate spectacular WOW HTML"""
    sys_info = get_system_info()
    animation = generate_animation_frames()
    random_color = f"hsl({random.randint(0, 360)}, 70%, 60%)"
    
    html = f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🚀 CGI WOW TEST 🚀</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Courier New', monospace;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%);
            color: #00ff00;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
            overflow-x: hidden;
        }}
        
        .container {{
            width: 100%;
            max-width: 900px;
            background: rgba(0, 255, 0, 0.05);
            border: 3px solid #00ff00;
            border-radius: 10px;
            padding: 40px;
            box-shadow: 0 0 30px rgba(0, 255, 0, 0.3),
                        0 0 60px rgba(0, 255, 0, 0.1),
                        inset 0 0 20px rgba(0, 255, 0, 0.05);
            animation: glowPulse 2s ease-in-out infinite;
        }}
        
        @keyframes glowPulse {{
            0%, 100% {{ box-shadow: 0 0 30px rgba(0, 255, 0, 0.3), inset 0 0 20px rgba(0, 255, 0, 0.05); }}
            50% {{ box-shadow: 0 0 50px rgba(0, 255, 0, 0.5), inset 0 0 30px rgba(0, 255, 0, 0.1); }}
        }}
        
        h1 {{
            text-align: center;
            font-size: 3em;
            margin-bottom: 20px;
            text-shadow: 0 0 10px #00ff00, 0 0 20px #00ff00;
            animation: float 3s ease-in-out infinite;
        }}
        
        @keyframes float {{
            0%, 100% {{ transform: translateY(0px); }}
            50% {{ transform: translateY(-10px); }}
        }}
        
        .subtitle {{
            text-align: center;
            font-size: 1.2em;
            margin-bottom: 30px;
            color: #00ffff;
            text-shadow: 0 0 10px #00ffff;
            animation: colorShift 4s ease-in-out infinite;
        }}
        
        @keyframes colorShift {{
            0% {{ color: #00ffff; }}
            50% {{ color: #ff00ff; }}
            100% {{ color: #00ffff; }}
        }}
        
        .animation {{
            text-align: center;
            font-size: 4em;
            margin: 30px 0;
            animation: spin 2s linear infinite;
        }}
        
        @keyframes spin {{
            0% {{ transform: rotate(0deg); }}
            100% {{ transform: rotate(360deg); }}
        }}
        
        .info-grid {{
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin: 40px 0;
        }}
        
        @media (max-width: 600px) {{
            .info-grid {{
                grid-template-columns: 1fr;
            }}
        }}
        
        .info-card {{
            background: rgba(0, 255, 0, 0.1);
            border: 2px solid {random_color};
            border-radius: 5px;
            padding: 15px;
            animation: slideIn 0.5s ease-out;
        }}
        
        @keyframes slideIn {{
            from {{
                opacity: 0;
                transform: translateX(-20px);
            }}
            to {{
                opacity: 1;
                transform: translateX(0);
            }}
        }}
        
        .info-label {{
            color: #00ffff;
            font-weight: bold;
            margin-bottom: 5px;
            text-shadow: 0 0 5px #00ffff;
        }}
        
        .info-value {{
            color: {random_color};
            font-size: 1.1em;
            font-weight: bold;
            word-break: break-all;
        }}
        
        .cgi-params {{
            background: rgba(0, 255, 0, 0.1);
            border: 2px solid #ff00ff;
            border-radius: 5px;
            padding: 20px;
            margin: 20px 0;
            max-height: 300px;
            overflow-y: auto;
        }}
        
        .param {{
            margin: 10px 0;
            padding: 10px;
            background: rgba(255, 0, 255, 0.05);
            border-left: 3px solid #ff00ff;
            padding-left: 15px;
        }}
        
        .param-key {{
            color: #ff00ff;
            font-weight: bold;
        }}
        
        .param-value {{
            color: #ffff00;
            margin-left: 10px;
            word-break: break-all;
        }}
        
        .button-group {{
            display: flex;
            gap: 10px;
            justify-content: center;
            margin: 30px 0;
            flex-wrap: wrap;
        }}
        
        button {{
            padding: 12px 24px;
            font-size: 1em;
            font-family: 'Courier New', monospace;
            background: linear-gradient(135deg, {random_color}, #ff00ff);
            color: #000;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.3s ease;
            text-shadow: 0 0 5px rgba(255, 255, 255, 0.5);
            box-shadow: 0 0 15px rgba(0, 255, 0, 0.3);
        }}
        
        button:hover {{
            transform: scale(1.1);
            box-shadow: 0 0 25px rgba(0, 255, 0, 0.6);
        }}
        
        button:active {{
            transform: scale(0.95);
        }}
        
        .console {{
            background: #000;
            border: 2px solid #00ff00;
            border-radius: 5px;
            padding: 15px;
            margin: 20px 0;
            font-size: 0.9em;
            line-height: 1.6;
            overflow-x: auto;
            max-height: 200px;
            overflow-y: auto;
        }}
        
        .console-line {{
            margin: 5px 0;
        }}
        
        .success {{
            color: #00ff00;
        }}
        
        .warning {{
            color: #ffff00;
        }}
        
        .error {{
            color: #ff0000;
        }}
        
        .info {{
            color: #00ffff;
        }}
        
        footer {{
            text-align: center;
            margin-top: 40px;
            padding-top: 20px;
            border-top: 2px solid #00ff00;
            font-size: 0.9em;
            color: #00ffff;
        }}
        
        .matrix {{
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
            opacity: 0.05;
            font-size: 20px;
            overflow: hidden;
        }}
        
        .loading-bar {{
            width: 100%;
            height: 20px;
            background: rgba(0, 255, 0, 0.2);
            border: 2px solid #00ff00;
            border-radius: 10px;
            overflow: hidden;
            margin: 20px 0;
        }}
        
        .loading-bar-fill {{
            height: 100%;
            background: linear-gradient(90deg, #00ff00, #00ffff, #ff00ff);
            width: 100%;
            animation: loading 2s ease-in-out infinite;
            box-shadow: 0 0 10px #00ff00;
        }}
        
        @keyframes loading {{
            0%, 100% {{ width: 100%; }}
            50% {{ width: 50%; }}
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 SUPER CGI TEST 🚀</h1>
        <div class="subtitle">Welcome to the most WOW CGI script ever!</div>
        
        <div class="animation">{animation}</div>
        <div class="loading-bar">
            <div class="loading-bar-fill"></div>
        </div>
        
        <div class="info-grid">
            <div class="info-card">
                <div class="info-label">⏰ Timestamp</div>
                <div class="info-value">{sys_info['timestamp']}</div>
            </div>
            <div class="info-card">
                <div class="info-label">🖥️ Hostname</div>
                <div class="info-value">{sys_info['hostname']}</div>
            </div>
            <div class="info-card">
                <div class="info-label">⬆️ System Uptime</div>
                <div class="info-value">{sys_info['uptime']}</div>
            </div>
            <div class="info-card">
                <div class="info-label">💾 Memory Usage</div>
                <div class="info-value">{sys_info['memory']}</div>
            </div>
            <div class="info-card">
                <div class="info-label">🔧 CPU Cores</div>
                <div class="info-value">{sys_info['cpu_count']}</div>
            </div>
            <div class="info-card">
                <div class="info-label">🌐 Request Method</div>
                <div class="info-value">{os.environ.get('REQUEST_METHOD', 'UNKNOWN')}</div>
            </div>
        </div>
        
        <div class="button-group">
            <button onclick="location.reload()">🔄 Refresh</button>
            <button onclick="copyToClipboard()">📋 Copy Info</button>
        </div>
        
        <div class="console">
            <div class="console-line success">✓ CGI Script loaded successfully</div>
            <div class="console-line info">ℹ Python Version: {sys.version.split()[0]}</div>
            <div class="console-line success">✓ System info retrieved</div>
            <div class="console-line warning">⚠ {len(os.environ)} environment variables available</div>
            <div class="console-line info">ℹ Working directory: {os.getcwd()}</div>
            <div class="console-line success">✓ Page generated in real-time</div>
        </div>
        
        <div class="cgi-params">
            <h2 style="color: #ff00ff; margin-bottom: 15px;">🔍 CGI Environment Variables</h2>
"""
    
    # Add CGI environment variables
    important_vars = [
        'REQUEST_METHOD', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH',
        'HTTP_HOST', 'HTTP_USER_AGENT', 'SCRIPT_NAME', 'SERVER_NAME',
        'SERVER_PORT', 'REMOTE_ADDR', 'PATH_INFO'
    ]
    
    for var in important_vars:
        value = os.environ.get(var, '(not set)')
        if len(str(value)) > 100:
            value = str(value)[:97] + '...'
        html += f"""            <div class="param">
                <span class="param-key">{var}:</span>
                <span class="param-value">{value}</span>
            </div>
"""
    
    html += f"""        </div>
        
        <footer>
            <div style="margin-bottom: 10px;">
                Generated by 🐍 Python CGI Magic ✨
            </div>
            <div>
                🌟 The most impressive webserv test ever created 🌟
            </div>
        </footer>
    </div>
    
    <script>
        function copyToClipboard() {{
            const text = document.body.innerText;
            navigator.clipboard.writeText(text).then(() => {{
                alert('✓ Information copied to clipboard!');
            }}).catch(() => {{
                alert('⚠ Copy failed!');
            }});
        }}
        
        // Add random matrix effect in background
        const matrix = document.querySelector('.matrix');
        if (matrix) {{
            const chars = '01アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲン';
            let html = '';
            for (let i = 0; i < 500; i++) {{
                html += chars[Math.floor(Math.random() * chars.length)];
                if (Math.random() > 0.9) html += '<br>';
            }}
            matrix.innerHTML = html;
        }}
    </script>
</body>
</html>"""
    
    return html

# Main CGI execution
if __name__ == '__main__':
    # print("Content-type: text/html; charset=utf-8\r\n\r\n", end='')
    print(generate_html())
