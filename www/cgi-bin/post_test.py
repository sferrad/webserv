#!/usr/bin/env python3
"""
Script CGI pour tester les requêtes POST
Gère les données de formulaire et les fichiers uploadés
"""

import os
import sys
import cgi
import cgitb
import json
from datetime import datetime

# Activer le debug CGI
cgitb.enable()

def print_headers():
    """Affiche les headers HTTP"""
    print("Content-Type: text/html; charset=utf-8")
    print()

def parse_post_data():
    """Parse les données POST du formulaire"""
    form = cgi.FieldStorage()
    
    # Récupérer toutes les données
    form_data = {}
    for key in form.keys():
        field = form[key]
        if field.filename:  # C'est un fichier
            form_data[key] = {
                'type': 'file',
                'filename': field.filename,
                'content_type': field.type,
                'size': len(field.value) if hasattr(field, 'value') else 0,
                'content': field.value[:100].decode('utf-8', errors='ignore') if hasattr(field, 'value') else ''
            }
        else:  # C'est une valeur normale
            form_data[key] = {
                'type': 'text',
                'value': field.value
            }
    
    return form_data

def get_environment_info():
    """Récupère les informations importantes de l'environnement CGI"""
    env_info = {}
    important_vars = [
        'REQUEST_METHOD', 'CONTENT_TYPE', 'CONTENT_LENGTH', 'QUERY_STRING',
        'HTTP_HOST', 'HTTP_USER_AGENT', 'HTTP_REFERER', 'HTTP_COOKIE',
        'SCRIPT_NAME', 'PATH_INFO', 'SERVER_NAME', 'SERVER_PORT',
        'REMOTE_ADDR', 'REMOTE_HOST', 'SERVER_SOFTWARE'
    ]
    
    for var in important_vars:
        env_info[var] = os.environ.get(var, '(not set)')
    
    return env_info

def read_raw_post_data():
    """Lit les données POST brutes"""
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    try:
        content_length = int(content_length)
    except ValueError:
        content_length = 0
    
    if content_length > 0:
        try:
            return sys.stdin.buffer.read(content_length)
        except:
            return b''
    return b''

def generate_html_response():
    """Génère la réponse HTML complète"""
    method = os.environ.get('REQUEST_METHOD', 'UNKNOWN')
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
    
    # Récupérer les informations
    env_info = get_environment_info()
    form_data = {}
    raw_data = b''
    
    if method == 'POST':
        try:
            form_data = parse_post_data()
            raw_data = read_raw_post_data()
        except Exception as e:
            form_data = {'error': str(e)}
    
    html = f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🚀 Test CGI POST 🚀</title>
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
            padding: 20px;
            line-height: 1.6;
        }}
        
        .container {{
            max-width: 1200px;
            margin: 0 auto;
            background: rgba(0, 0, 0, 0.7);
            border: 2px solid #00ff00;
            border-radius: 10px;
            padding: 30px;
            box-shadow: 0 0 30px rgba(0, 255, 0, 0.3);
        }}
        
        h1, h2, h3 {{
            color: #00ffff;
            text-shadow: 0 0 10px #00ffff;
            margin-bottom: 15px;
        }}
        
        .section {{
            margin: 20px 0;
            padding: 15px;
            background: rgba(0, 255, 0, 0.05);
            border: 1px solid rgba(0, 255, 0, 0.3);
            border-radius: 5px;
        }}
        
        .status {{
            text-align: center;
            padding: 20px;
            font-size: 1.5em;
            font-weight: bold;
        }}
        
        .method-get {{
            color: #00ff00;
        }}
        
        .method-post {{
            color: #ff6600;
        }}
        
        .method-other {{
            color: #ff0066;
        }}
        
        .info-grid {{
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin: 20px 0;
        }}
        
        @media (max-width: 768px) {{
            .info-grid {{
                grid-template-columns: 1fr;
            }}
        }}
        
        .info-item {{
            background: rgba(0, 0, 0, 0.5);
            padding: 10px;
            border-radius: 5px;
            border-left: 3px solid #00ffff;
        }}
        
        .info-label {{
            color: #ffff00;
            font-weight: bold;
        }}
        
        .info-value {{
            color: #ffffff;
            word-break: break-all;
        }}
        
        .data-container {{
            background: rgba(0, 0, 0, 0.8);
            border: 1px solid #ff00ff;
            border-radius: 5px;
            padding: 15px;
            margin: 10px 0;
            max-height: 300px;
            overflow-y: auto;
        }}
        
        pre {{
            color: #00ff00;
            font-size: 0.9em;
            white-space: pre-wrap;
            word-wrap: break-word;
        }}
        
        .form-data-item {{
            margin: 10px 0;
            padding: 10px;
            background: rgba(255, 0, 255, 0.1);
            border-radius: 3px;
        }}
        
        .file-info {{
            background: rgba(255, 165, 0, 0.2);
            padding: 8px;
            border-radius: 3px;
            margin-top: 5px;
        }}
        
        .button {{
            display: inline-block;
            padding: 10px 20px;
            background: linear-gradient(45deg, #00ff00, #00ffff);
            color: #000;
            text-decoration: none;
            border-radius: 5px;
            font-weight: bold;
            margin: 5px;
            border: none;
            cursor: pointer;
        }}
        
        .button:hover {{
            transform: scale(1.05);
            box-shadow: 0 0 15px rgba(0, 255, 0, 0.5);
        }}
        
        .timestamp {{
            text-align: center;
            color: #ffff00;
            font-size: 0.9em;
            margin-top: 20px;
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
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 Test CGI POST Results 🚀</h1>
        
        <div class="status">
            <span class="method-{method.lower()}">
                {'✅ POST Request Detected!' if method == 'POST' else '📝 GET Request' if method == 'GET' else '❓ Unknown Method'}
            </span>
        </div>
        
        <div class="section">
            <h2>📊 Request Information</h2>
            <div class="info-grid">
                <div class="info-item">
                    <div class="info-label">Method:</div>
                    <div class="info-value">{method}</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Timestamp:</div>
                    <div class="info-value">{timestamp}</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Content Type:</div>
                    <div class="info-value">{env_info.get('CONTENT_TYPE', 'N/A')}</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Content Length:</div>
                    <div class="info-value">{env_info.get('CONTENT_LENGTH', 'N/A')} bytes</div>
                </div>
            </div>
        </div>"""

    # Afficher les données du formulaire si c'est un POST
    if method == 'POST' and form_data:
        html += f"""
        <div class="section">
            <h2>📝 Form Data Received</h2>
            <div class="data-container">"""
        
        if 'error' in form_data:
            html += f'<div class="error">Error parsing form data: {form_data["error"]}</div>'
        else:
            for key, data in form_data.items():
                html += f'<div class="form-data-item">'
                html += f'<strong class="info-label">{key}:</strong><br>'
                
                if data['type'] == 'file':
                    html += f'''<div class="file-info">
                        <div>📁 Filename: {data.get('filename', 'N/A')}</div>
                        <div>📄 Content Type: {data.get('content_type', 'N/A')}</div>
                        <div>📏 Size: {data.get('size', 0)} bytes</div>
                        <div>📖 Content Preview: {data.get('content', '')[:100]}{'...' if len(data.get('content', '')) > 100 else ''}</div>
                    </div>'''
                else:
                    html += f'<div class="info-value">{data.get("value", "")}</div>'
                
                html += '</div>'
        
        html += """</div>
        </div>"""

    # Afficher les données brutes si disponibles
    if raw_data:
        html += f"""
        <div class="section">
            <h2>🔍 Raw POST Data</h2>
            <div class="data-container">
                <pre>{raw_data[:1000].decode('utf-8', errors='ignore')}{'...' if len(raw_data) > 1000 else ''}</pre>
            </div>
        </div>"""

    # Variables d'environnement importantes
    html += f"""
        <div class="section">
            <h2>🌍 Environment Variables</h2>
            <div class="data-container">"""
    
    for key, value in env_info.items():
        color_class = "success" if value != "(not set)" else "warning"
        html += f'<div class="info-item"><span class="info-label">{key}:</span> <span class="{color_class}">{value}</span></div>'
    
    html += f"""</div>
        </div>
        
        <div class="section" style="text-align: center;">
            <h3>🔗 Quick Actions</h3>
            <a href="/cgi-bin/post_test_form.html" class="button">📝 Go to Test Form</a>
            <a href="/cgi-bin/index.html" class="button">🏠 Back to Index</a>
            <button onclick="location.reload()" class="button">🔄 Refresh</button>
        </div>
        
        <div class="timestamp">
            Generated at {timestamp} by Python CGI
        </div>
    </div>
    
    <script>
        console.log('CGI POST Test Results loaded');
        console.log('Request method:', '{method}');
        console.log('Content length:', '{env_info.get('CONTENT_LENGTH', '0')}');
    </script>
</body>
</html>"""
    
    return html

# Exécution principale
if __name__ == '__main__':
    print_headers()
    print(generate_html_response())