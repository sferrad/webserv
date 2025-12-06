#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
from datetime import datetime, timedelta
from urllib.parse import parse_qs

def parse_cookies():
    """Parse les cookies depuis HTTP_COOKIE"""
    cookies = {}
    cookie_string = os.environ.get('HTTP_COOKIE', '')
    if cookie_string:
        for cookie in cookie_string.split(';'):
            cookie = cookie.strip()
            if '=' in cookie:
                name, value = cookie.split('=', 1)
                cookies[name] = value
    return cookies

def get_post_data():
    """Récupère les données POST"""
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    if content_length and content_length.isdigit():
        return sys.stdin.read(int(content_length))
    return ''

def main():
    # Récupérer les cookies existants
    cookies = parse_cookies()
    
    # Variables pour la gestion des cookies
    new_cookie_header = ""
    message = ""
    action = ""
    
    # Traiter les données POST
    if os.environ.get('REQUEST_METHOD') == 'POST':
        post_data = get_post_data()
        params = parse_qs(post_data)
        
        action = params.get('action', [''])[0]
        
        if action == 'set':
            cookie_name = params.get('cookie_name', [''])[0]
            cookie_value = params.get('cookie_value', [''])[0]
            
            if cookie_name and cookie_value:
                # Créer un cookie qui expire dans 1 heure
                expires = datetime.utcnow() + timedelta(hours=1)
                expires_str = expires.strftime('%a, %d %b %Y %H:%M:%S GMT')
                new_cookie_header = f"Set-Cookie: {cookie_name}={cookie_value}; Expires={expires_str}; Path=/\r\n"
                message = f"Cookie '{cookie_name}' défini avec succès !"
                cookies[cookie_name] = cookie_value
            else:
                message = "Erreur: nom et valeur requis"
        
        elif action == 'delete':
            cookie_name = params.get('cookie_name', [''])[0]
            if cookie_name:
                # Supprimer un cookie en définissant une date d'expiration passée
                new_cookie_header = f"Set-Cookie: {cookie_name}=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/\r\n"
                message = f"Cookie '{cookie_name}' supprimé !"
                if cookie_name in cookies:
                    del cookies[cookie_name]
            else:
                message = "Erreur: nom du cookie requis"
    
    # En-têtes HTTP
    print("Content-Type: text/html; charset=utf-8")
    if new_cookie_header:
        print(new_cookie_header, end='')
    print()  # Ligne vide pour séparer les en-têtes du corps
    
    # Page HTML stylisée
    html = f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Gestionnaire de Cookies - CGI</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
        }}
        
        .container {{
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            max-width: 800px;
            width: 100%;
            padding: 40px;
            animation: fadeIn 0.5s ease-in;
        }}
        
        @keyframes fadeIn {{
            from {{
                opacity: 0;
                transform: translateY(-20px);
            }}
            to {{
                opacity: 1;
                transform: translateY(0);
            }}
        }}
        
        h1 {{
            color: #667eea;
            text-align: center;
            margin-bottom: 10px;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1);
        }}
        
        .subtitle {{
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 1.1em;
        }}
        
        .message {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 15px 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            text-align: center;
            animation: slideDown 0.3s ease-out;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        }}
        
        @keyframes slideDown {{
            from {{
                opacity: 0;
                transform: translateY(-10px);
            }}
            to {{
                opacity: 1;
                transform: translateY(0);
            }}
        }}
        
        .section {{
            background: #f8f9fa;
            padding: 25px;
            border-radius: 15px;
            margin-bottom: 25px;
            border: 2px solid #e9ecef;
            transition: all 0.3s ease;
        }}
        
        .section:hover {{
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.2);
            border-color: #667eea;
        }}
        
        .section h2 {{
            color: #764ba2;
            margin-bottom: 15px;
            font-size: 1.5em;
            display: flex;
            align-items: center;
            gap: 10px;
        }}
        
        .section h2::before {{
            content: "🍪";
            font-size: 1.2em;
        }}
        
        .cookie-list {{
            background: white;
            padding: 15px;
            border-radius: 10px;
            margin-top: 10px;
        }}
        
        .cookie-item {{
            background: linear-gradient(135deg, #667eea15 0%, #764ba215 100%);
            padding: 12px 15px;
            margin: 8px 0;
            border-radius: 8px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            transition: transform 0.2s ease;
        }}
        
        .cookie-item:hover {{
            transform: translateX(5px);
        }}
        
        .cookie-name {{
            font-weight: bold;
            color: #667eea;
        }}
        
        .cookie-value {{
            color: #555;
            font-family: 'Courier New', monospace;
        }}
        
        .form-group {{
            margin-bottom: 20px;
        }}
        
        label {{
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
        }}
        
        input[type="text"] {{
            width: 100%;
            padding: 12px 15px;
            border: 2px solid #e9ecef;
            border-radius: 8px;
            font-size: 16px;
            transition: all 0.3s ease;
        }}
        
        input[type="text"]:focus {{
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }}
        
        .button-group {{
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
        }}
        
        button {{
            flex: 1;
            min-width: 150px;
            padding: 12px 25px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }}
        
        .btn-primary {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
        }}
        
        .btn-primary:hover {{
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6);
        }}
        
        .btn-danger {{
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            color: white;
            box-shadow: 0 4px 15px rgba(245, 87, 108, 0.4);
        }}
        
        .btn-danger:hover {{
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(245, 87, 108, 0.6);
        }}
        
        .info-box {{
            background: #e7f3ff;
            border-left: 4px solid #2196F3;
            padding: 15px;
            border-radius: 5px;
            margin-top: 20px;
        }}
        
        .info-box strong {{
            color: #2196F3;
        }}
        
        .env-info {{
            background: #fff3cd;
            border-left: 4px solid #ffc107;
            padding: 10px;
            border-radius: 5px;
            margin-top: 10px;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
            color: #856404;
        }}
        
        .footer {{
            text-align: center;
            margin-top: 30px;
            color: #666;
            font-size: 0.9em;
        }}
        
        .empty-state {{
            text-align: center;
            padding: 30px;
            color: #999;
            font-style: italic;
        }}
        
        @media (max-width: 600px) {{
            .container {{
                padding: 20px;
            }}
            
            h1 {{
                font-size: 2em;
            }}
            
            .button-group {{
                flex-direction: column;
            }}
            
            button {{
                width: 100%;
            }}
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🍪 Cookie Manager</h1>
        <p class="subtitle">Gestionnaire de cookies via CGI Python</p>
        
        {f'<div class="message">{message}</div>' if message else ''}
        
        <!-- Liste des cookies actuels -->
        <div class="section">
            <h2>Cookies Actuels</h2>
            <div class="cookie-list">
"""
    
    if cookies:
        for name, value in cookies.items():
            html += f"""
                <div class="cookie-item">
                    <div>
                        <span class="cookie-name">{name}</span>: 
                        <span class="cookie-value">{value}</span>
                    </div>
                </div>
"""
    else:
        html += """
                <div class="empty-state">
                    Aucun cookie défini pour le moment
                </div>
"""
    
    html += f"""
            </div>
        </div>
        
        <!-- Formulaire pour créer un cookie -->
        <div class="section">
            <h2>Créer un Cookie</h2>
            <form method="POST" action="/cgi-bin/cookie_test.py">
                <input type="hidden" name="action" value="set">
                <div class="form-group">
                    <label for="cookie_name">Nom du cookie:</label>
                    <input type="text" id="cookie_name" name="cookie_name" 
                           placeholder="Ex: user_id" required>
                </div>
                <div class="form-group">
                    <label for="cookie_value">Valeur du cookie:</label>
                    <input type="text" id="cookie_value" name="cookie_value" 
                           placeholder="Ex: 12345" required>
                </div>
                <div class="button-group">
                    <button type="submit" class="btn-primary">✨ Créer le Cookie</button>
                </div>
            </form>
            <div class="info-box">
                <strong>ℹ️ Info:</strong> Le cookie sera valide pendant 1 heure
            </div>
        </div>
        
        <!-- Formulaire pour supprimer un cookie -->
        <div class="section">
            <h2>Supprimer un Cookie</h2>
            <form method="POST" action="/cgi-bin/cookie_test.py">
                <input type="hidden" name="action" value="delete">
                <div class="form-group">
                    <label for="delete_cookie_name">Nom du cookie à supprimer:</label>
                    <input type="text" id="delete_cookie_name" name="cookie_name" 
                           placeholder="Ex: user_id" required>
                </div>
                <div class="button-group">
                    <button type="submit" class="btn-danger">🗑️ Supprimer le Cookie</button>
                </div>
            </form>
        </div>
        
        <!-- Informations techniques -->
        <div class="section">
            <h2>Informations de l'Environnement CGI</h2>
            <div class="env-info">
                <strong>REQUEST_METHOD:</strong> {os.environ.get('REQUEST_METHOD', 'N/A')}<br>
                <strong>SERVER_SOFTWARE:</strong> {os.environ.get('SERVER_SOFTWARE', 'N/A')}<br>
                <strong>REMOTE_ADDR:</strong> {os.environ.get('REMOTE_ADDR', 'N/A')}<br>
                <strong>HTTP_USER_AGENT:</strong> {os.environ.get('HTTP_USER_AGENT', 'N/A')[:50]}...<br>
                <strong>SCRIPT_NAME:</strong> {os.environ.get('SCRIPT_NAME', 'N/A')}<br>
                <strong>HTTP_COOKIE:</strong> {os.environ.get('HTTP_COOKIE', '(vide)')}
            </div>
        </div>
        
        <div class="footer">
            <p>🚀 CGI Cookie Manager | Webserv Project | {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        </div>
    </div>
</body>
</html>
"""
    
    print(html)

if __name__ == "__main__":
    main()
