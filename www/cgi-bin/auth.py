#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import json
import hashlib
import secrets
from datetime import datetime, timedelta
from urllib.parse import parse_qs

# Fichiers de stockage
USERS_FILE = "/home/sferrad/42/CommonCore/webserv/www/cgi-bin/users.json"
SESSIONS_FILE = "/home/sferrad/42/CommonCore/webserv/www/cgi-bin/sessions.json"

def load_json_file(filepath, default=None):
    """Charge un fichier JSON"""
    if default is None:
        default = {}
    try:
        if os.path.exists(filepath):
            with open(filepath, 'r') as f:
                return json.load(f)
    except:
        pass
    return default

def save_json_file(filepath, data):
    """Sauvegarde un fichier JSON"""
    try:
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
        return True
    except:
        return False

def hash_password(password):
    """Hash un mot de passe avec SHA256"""
    return hashlib.sha256(password.encode()).hexdigest()

def generate_session_token():
    """Génère un token de session sécurisé"""
    return secrets.token_hex(32)

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

def get_current_user():
    """Récupère l'utilisateur actuel depuis la session"""
    cookies = parse_cookies()
    session_token = cookies.get('session_token')
    
    if not session_token:
        return None
    
    sessions = load_json_file(SESSIONS_FILE, {})
    session = sessions.get(session_token)
    
    if not session:
        return None
    
    # Vérifier l'expiration
    expires = datetime.fromisoformat(session['expires'])
    if datetime.now() > expires:
        del sessions[session_token]
        save_json_file(SESSIONS_FILE, sessions)
        return None
    
    return session['username']

def create_session(username):
    """Crée une nouvelle session pour un utilisateur"""
    token = generate_session_token()
    expires = datetime.now() + timedelta(hours=24)
    
    sessions = load_json_file(SESSIONS_FILE, {})
    sessions[token] = {
        'username': username,
        'created': datetime.now().isoformat(),
        'expires': expires.isoformat()
    }
    save_json_file(SESSIONS_FILE, sessions)
    
    # Créer le cookie
    expires_str = expires.strftime('%a, %d %b %Y %H:%M:%S GMT')
    return f"Set-Cookie: session_token={token}; Expires={expires_str}; Path=/; HttpOnly\r\n"

def logout():
    """Déconnecte l'utilisateur"""
    cookies = parse_cookies()
    session_token = cookies.get('session_token')
    
    if session_token:
        sessions = load_json_file(SESSIONS_FILE, {})
        if session_token in sessions:
            del sessions[session_token]
            save_json_file(SESSIONS_FILE, sessions)
    
    return "Set-Cookie: session_token=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/\r\n"

def register_user(username, password, email):
    """Enregistre un nouvel utilisateur"""
    users = load_json_file(USERS_FILE, {})
    
    # Vérifier si l'utilisateur existe déjà
    if username in users:
        return False, "Ce nom d'utilisateur est déjà pris"
    
    # Créer l'utilisateur
    users[username] = {
        'password': hash_password(password),
        'email': email,
        'created': datetime.now().isoformat()
    }
    
    if save_json_file(USERS_FILE, users):
        return True, "Inscription réussie !"
    return False, "Erreur lors de l'enregistrement"

def login_user(username, password):
    """Connecte un utilisateur"""
    users = load_json_file(USERS_FILE, {})
    
    if username not in users:
        return False, "Nom d'utilisateur ou mot de passe incorrect"
    
    user_data = users[username]
    # Gérer l'ancien format (string simple) et le nouveau format (dict)
    if isinstance(user_data, str):
        # Ancien format: juste le mot de passe en clair
        if user_data != password:
            return False, "Nom d'utilisateur ou mot de passe incorrect"
    else:
        # Nouveau format: dict avec password hashé
        if user_data['password'] != hash_password(password):
            return False, "Nom d'utilisateur ou mot de passe incorrect"
    
    return True, "Connexion réussie !"

def get_post_data():
    """Récupère les données POST"""
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    if content_length and content_length.isdigit():
        return sys.stdin.read(int(content_length))
    return ''

def main():
    current_user = get_current_user()
    message = ""
    message_type = ""
    cookie_header = ""
    
    # Traiter les actions POST
    if os.environ.get('REQUEST_METHOD') == 'POST':
        post_data = get_post_data()
        params = parse_qs(post_data)
        
        action = params.get('action', [''])[0]
        
        if action == 'register':
            username = params.get('username', [''])[0].strip()
            password = params.get('password', [''])[0]
            email = params.get('email', [''])[0].strip()
            
            if username and password and email:
                success, msg = register_user(username, password, email)
                message = msg
                message_type = "success" if success else "error"
                
                if success:
                    cookie_header = create_session(username)
                    current_user = username
            else:
                message = "Tous les champs sont requis"
                message_type = "error"
        
        elif action == 'login':
            username = params.get('username', [''])[0].strip()
            password = params.get('password', [''])[0]
            
            if username and password:
                success, msg = login_user(username, password)
                message = msg
                message_type = "success" if success else "error"
                
                if success:
                    cookie_header = create_session(username)
                    current_user = username
            else:
                message = "Nom d'utilisateur et mot de passe requis"
                message_type = "error"
        
        elif action == 'logout':
            cookie_header = logout()
            current_user = None
            message = "Déconnexion réussie"
            message_type = "success"
    
    # En-têtes HTTP
    print("Content-Type: text/html; charset=utf-8")
    if cookie_header:
        print(cookie_header, end='')
    print()
    
    # Page HTML
    if current_user:
        # Page pour utilisateur connecté
        users = load_json_file(USERS_FILE, {})
        user_info = users.get(current_user, {})
        
        html = f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard - {current_user}</title>
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
            max-width: 600px;
            width: 100%;
            padding: 40px;
            animation: fadeIn 0.5s ease-in;
        }}
        
        @keyframes fadeIn {{
            from {{ opacity: 0; transform: translateY(-20px); }}
            to {{ opacity: 1; transform: translateY(0); }}
        }}
        
        h1 {{
            color: #667eea;
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.5em;
        }}
        
        .user-card {{
            background: linear-gradient(135deg, #667eea15 0%, #764ba215 100%);
            padding: 30px;
            border-radius: 15px;
            margin-bottom: 30px;
            text-align: center;
        }}
        
        .avatar {{
            width: 100px;
            height: 100px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            border-radius: 50%;
            margin: 0 auto 20px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 3em;
            color: white;
        }}
        
        .username {{
            font-size: 2em;
            color: #333;
            margin-bottom: 10px;
            font-weight: bold;
        }}
        
        .user-email {{
            color: #666;
            font-size: 1.1em;
        }}
        
        .info-section {{
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
        }}
        
        .info-row {{
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid #e9ecef;
        }}
        
        .info-row:last-child {{
            border-bottom: none;
        }}
        
        .info-label {{
            font-weight: bold;
            color: #667eea;
        }}
        
        .info-value {{
            color: #555;
        }}
        
        .message {{
            padding: 15px 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            text-align: center;
            animation: slideDown 0.3s ease-out;
        }}
        
        .message.success {{
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }}
        
        .btn {{
            width: 100%;
            padding: 15px;
            border: none;
            border-radius: 10px;
            font-size: 1.1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 1px;
        }}
        
        .btn-logout {{
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            color: white;
            box-shadow: 0 4px 15px rgba(245, 87, 108, 0.4);
        }}
        
        .btn-logout:hover {{
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(245, 87, 108, 0.6);
        }}
        
        @keyframes slideDown {{
            from {{ opacity: 0; transform: translateY(-10px); }}
            to {{ opacity: 1; transform: translateY(0); }}
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>👋 Bienvenue !</h1>
        
        {f'<div class="message {message_type}">{message}</div>' if message else ''}
        
        <div class="user-card">
            <div class="avatar">{current_user[0].upper()}</div>
            <div class="username">{current_user}</div>
            <div class="user-email">{user_info.get('email', 'N/A')}</div>
        </div>
        
        <div class="info-section">
            <div class="info-row">
                <span class="info-label">📅 Membre depuis:</span>
                <span class="info-value">{user_info.get('created', 'N/A')[:10]}</span>
            </div>
            <div class="info-row">
                <span class="info-label">🌐 IP:</span>
                <span class="info-value">{os.environ.get('REMOTE_ADDR', 'N/A')}</span>
            </div>
            <div class="info-row">
                <span class="info-label">🕐 Date actuelle:</span>
                <span class="info-value">{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</span>
            </div>
        </div>
        
        <form method="POST" action="/cgi-bin/auth.py">
            <input type="hidden" name="action" value="logout">
            <button type="submit" class="btn btn-logout">🚪 Se Déconnecter</button>
        </form>
        
        <div style="text-align: center; margin-top: 20px;">
            <a href="/cgi-bin/snake.py" style="display: inline-block; padding: 15px 30px; background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%); color: white; text-decoration: none; border-radius: 10px; font-weight: 600; transition: all 0.3s ease;" onmouseover="this.style.transform='translateY(-2px)'" onmouseout="this.style.transform='translateY(0)'">🎮 Jouer à Snake</a>
        </div>
    </div>
</body>
</html>"""
    else:
        # Page de login/register
        html = f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Connexion / Inscription</title>
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
            max-width: 450px;
            width: 100%;
            overflow: hidden;
            animation: fadeIn 0.5s ease-in;
        }}
        
        @keyframes fadeIn {{
            from {{ opacity: 0; transform: translateY(-20px); }}
            to {{ opacity: 1; transform: translateY(0); }}
        }}
        
        .header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 40px 30px;
            text-align: center;
            color: white;
        }}
        
        .header h1 {{
            font-size: 2.5em;
            margin-bottom: 10px;
        }}
        
        .header p {{
            opacity: 0.9;
            font-size: 1.1em;
        }}
        
        .tabs {{
            display: flex;
            background: #f8f9fa;
        }}
        
        .tab {{
            flex: 1;
            padding: 20px;
            text-align: center;
            cursor: pointer;
            border: none;
            background: transparent;
            font-size: 1.1em;
            font-weight: 600;
            color: #666;
            transition: all 0.3s ease;
            border-bottom: 3px solid transparent;
        }}
        
        .tab:hover {{
            background: #e9ecef;
        }}
        
        .tab.active {{
            color: #667eea;
            background: white;
            border-bottom-color: #667eea;
        }}
        
        .form-container {{
            padding: 40px;
        }}
        
        .form {{
            display: none;
        }}
        
        .form.active {{
            display: block;
            animation: slideIn 0.3s ease-out;
        }}
        
        @keyframes slideIn {{
            from {{ opacity: 0; transform: translateX(-10px); }}
            to {{ opacity: 1; transform: translateX(0); }}
        }}
        
        .form-group {{
            margin-bottom: 25px;
        }}
        
        label {{
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
            font-size: 0.95em;
        }}
        
        input[type="text"],
        input[type="password"],
        input[type="email"] {{
            width: 100%;
            padding: 12px 15px;
            border: 2px solid #e9ecef;
            border-radius: 8px;
            font-size: 16px;
            transition: all 0.3s ease;
        }}
        
        input:focus {{
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }}
        
        .btn {{
            width: 100%;
            padding: 15px;
            border: none;
            border-radius: 10px;
            font-size: 1.1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 1px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
        }}
        
        .btn:hover {{
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(102, 126, 234, 0.6);
        }}
        
        .message {{
            padding: 15px 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            text-align: center;
            animation: slideDown 0.3s ease-out;
        }}
        
        .message.success {{
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }}
        
        .message.error {{
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }}
        
        @keyframes slideDown {{
            from {{ opacity: 0; transform: translateY(-10px); }}
            to {{ opacity: 1; transform: translateY(0); }}
        }}
        
        .divider {{
            text-align: center;
            margin: 20px 0;
            color: #999;
            position: relative;
        }}
        
        .divider::before,
        .divider::after {{
            content: '';
            position: absolute;
            top: 50%;
            width: 40%;
            height: 1px;
            background: #ddd;
        }}
        
        .divider::before {{ left: 0; }}
        .divider::after {{ right: 0; }}
        
        @media (max-width: 600px) {{
            .container {{
                margin: 10px;
            }}
            
            .header h1 {{
                font-size: 2em;
            }}
            
            .form-container {{
                padding: 30px 20px;
            }}
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔐 Authentification</h1>
            <p>Webserv Auth System</p>
        </div>
        
        <div class="tabs">
            <button class="tab active" onclick="showTab('login')">Connexion</button>
            <button class="tab" onclick="showTab('register')">Inscription</button>
        </div>
        
        <div class="form-container">
            {f'<div class="message {message_type}">{message}</div>' if message else ''}
            
            <!-- Formulaire de connexion -->
            <div id="login-form" class="form active">
                <form method="POST" action="/cgi-bin/auth.py">
                    <input type="hidden" name="action" value="login">
                    
                    <div class="form-group">
                        <label for="login-username">👤 Nom d'utilisateur</label>
                        <input type="text" id="login-username" name="username" 
                               placeholder="Votre nom d'utilisateur" required autofocus>
                    </div>
                    
                    <div class="form-group">
                        <label for="login-password">🔒 Mot de passe</label>
                        <input type="password" id="login-password" name="password" 
                               placeholder="Votre mot de passe" required>
                    </div>
                    
                    <button type="submit" class="btn">🚀 Se Connecter</button>
                </form>
            </div>
            
            <!-- Formulaire d'inscription -->
            <div id="register-form" class="form">
                <form method="POST" action="/cgi-bin/auth.py">
                    <input type="hidden" name="action" value="register">
                    
                    <div class="form-group">
                        <label for="register-username">👤 Nom d'utilisateur</label>
                        <input type="text" id="register-username" name="username" 
                               placeholder="Choisissez un nom d'utilisateur" required>
                    </div>
                    
                    <div class="form-group">
                        <label for="register-email">📧 Email</label>
                        <input type="email" id="register-email" name="email" 
                               placeholder="votre@email.com" required>
                    </div>
                    
                    <div class="form-group">
                        <label for="register-password">🔒 Mot de passe</label>
                        <input type="password" id="register-password" name="password" 
                               placeholder="Choisissez un mot de passe" required>
                    </div>
                    
                    <button type="submit" class="btn">✨ Créer un Compte</button>
                </form>
            </div>
        </div>
    </div>
    
    <script>
        function showTab(tabName) {{
            // Update tabs
            const tabs = document.querySelectorAll('.tab');
            tabs.forEach(tab => tab.classList.remove('active'));
            event.target.classList.add('active');
            
            // Update forms
            const forms = document.querySelectorAll('.form');
            forms.forEach(form => form.classList.remove('active'));
            document.getElementById(tabName + '-form').classList.add('active');
        }}
    </script>
</body>
</html>"""
    
    print(html)

if __name__ == "__main__":
    main()
