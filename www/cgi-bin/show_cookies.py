#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
from datetime import datetime

def main():
    # En-têtes HTTP
    print("Content-Type: text/html; charset=utf-8")
    print()
    
    # Récupérer les cookies depuis les variables d'environnement CGI
    cookie_string = os.environ.get('HTTP_COOKIE', '')
    
    cookies = {}
    if cookie_string:
        for cookie in cookie_string.split(';'):
            cookie = cookie.strip()
            if '=' in cookie:
                name, value = cookie.split('=', 1)
                cookies[name] = value
    
    # Page HTML montrant les cookies reçus par le serveur
    html = f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Cookies reçus par le serveur</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
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
            max-width: 700px;
            width: 100%;
            padding: 40px;
            animation: fadeIn 0.5s ease-in;
        }}
        
        @keyframes fadeIn {{
            from {{ opacity: 0; transform: scale(0.95); }}
            to {{ opacity: 1; transform: scale(1); }}
        }}
        
        h1 {{
            color: #11998e;
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.5em;
        }}
        
        .server-info {{
            background: linear-gradient(135deg, #11998e15 0%, #38ef7d15 100%);
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 30px;
        }}
        
        .server-info h2 {{
            color: #11998e;
            margin-bottom: 15px;
        }}
        
        .cookie-table {{
            width: 100%;
            border-collapse: collapse;
            margin-top: 15px;
        }}
        
        .cookie-table th,
        .cookie-table td {{
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #e9ecef;
        }}
        
        .cookie-table th {{
            background: #11998e;
            color: white;
            font-weight: 600;
        }}
        
        .cookie-table tr:hover {{
            background: #f8f9fa;
        }}
        
        .cookie-name {{
            font-weight: bold;
            color: #11998e;
        }}
        
        .cookie-value {{
            font-family: 'Courier New', monospace;
            color: #555;
        }}
        
        .empty-state {{
            text-align: center;
            color: #999;
            padding: 40px;
            font-style: italic;
        }}
        
        .timestamp {{
            text-align: center;
            color: #666;
            margin-top: 20px;
            font-size: 0.9em;
        }}
        
        .btn {{
            display: inline-block;
            width: 100%;
            padding: 15px;
            background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
            color: white;
            text-align: center;
            text-decoration: none;
            border-radius: 10px;
            font-weight: 600;
            margin-top: 20px;
            transition: all 0.3s ease;
            box-shadow: 0 4px 15px rgba(17, 153, 142, 0.4);
        }}
        
        .btn:hover {{
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(17, 153, 142, 0.6);
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
    </style>
</head>
<body>
    <div class="container">
        <h1>🔍 Cookies côté serveur</h1>
        
        <div class="server-info">
            <h2>📡 Informations reçues par le serveur C++</h2>
            
            <p><strong>HTTP_COOKIE:</strong> {os.environ.get('HTTP_COOKIE', '(vide)')}</p>
            
            <table class="cookie-table">
                <thead>
                    <tr>
                        <th>Nom du Cookie</th>
                        <th>Valeur</th>
                    </tr>
                </thead>
                <tbody>
"""
    
    if cookies:
        for name, value in cookies.items():
            html += f"""
                    <tr>
                        <td class="cookie-name">{name}</td>
                        <td class="cookie-value">{value}</td>
                    </tr>
"""
    else:
        html += """
                    <tr>
                        <td colspan="2" class="empty-state">
                            Aucun cookie reçu par le serveur
                        </td>
                    </tr>
"""
    
    html += f"""
                </tbody>
            </table>
        </div>
        
        <div class="info-box">
            <strong>ℹ️ Comment ça marche:</strong><br>
            Le serveur WebServ en C++ parse automatiquement l'en-tête <code>Cookie:</code> de la requête HTTP
            et le transmet au script CGI via la variable d'environnement <code>HTTP_COOKIE</code>.
            <br><br>
            Les fonctions C++ disponibles:
            <ul style="margin-top: 10px; padding-left: 20px;">
                <li><code>parseCookies()</code> - Parse les cookies reçus</li>
                <li><code>getCookie(name)</code> - Récupère un cookie spécifique</li>
                <li><code>hasCookie(name)</code> - Vérifie l'existence</li>
            </ul>
        </div>
        
        <a href="/test_cookies_cpp.html" class="btn">← Retour au test JavaScript</a>
        
        <div class="timestamp">
            Généré le {datetime.now().strftime('%Y-%m-%d à %H:%M:%S')}
        </div>
    </div>
</body>
</html>"""
    
    print(html)

if __name__ == "__main__":
    main()
