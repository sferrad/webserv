#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import json
from datetime import datetime
from urllib.parse import parse_qs

# Fichiers de stockage
SESSIONS_FILE = "/home/sferrad/42/CommonCore/webserv/www/cgi-bin/sessions.json"
SCORES_FILE = "/home/sferrad/42/CommonCore/webserv/www/cgi-bin/snake_scores.json"

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

def save_score(username, score):
    """Sauvegarde le score d'un joueur"""
    scores = load_json_file(SCORES_FILE, {})
    
    if username not in scores:
        scores[username] = {
            'best_score': score,
            'total_games': 1,
            'last_played': datetime.now().isoformat()
        }
    else:
        scores[username]['total_games'] += 1
        scores[username]['last_played'] = datetime.now().isoformat()
        if score > scores[username]['best_score']:
            scores[username]['best_score'] = score
    
    save_json_file(SCORES_FILE, scores)

def get_leaderboard():
    """Récupère le classement"""
    scores = load_json_file(SCORES_FILE, {})
    leaderboard = []
    
    for username, data in scores.items():
        leaderboard.append({
            'username': username,
            'score': data['best_score'],
            'games': data['total_games']
        })
    
    leaderboard.sort(key=lambda x: x['score'], reverse=True)
    return leaderboard[:10]  # Top 10

def get_post_data():
    """Récupère les données POST"""
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    if content_length and content_length.isdigit():
        return sys.stdin.read(int(content_length))
    return ''

def main():
    current_user = get_current_user()
    
    # Redirection si non connecté
    if not current_user:
        print("Content-Type: text/html; charset=utf-8")
        print("Status: 302 Found")
        print("Location: /cgi-bin/auth.py")
        print()
        return
    
    # Traiter la sauvegarde de score
    if os.environ.get('REQUEST_METHOD') == 'POST':
        post_data = get_post_data()
        params = parse_qs(post_data)
        
        action = params.get('action', [''])[0]
        if action == 'save_score':
            score = params.get('score', ['0'])[0]
            if score.isdigit():
                save_score(current_user, int(score))
    
    leaderboard = get_leaderboard()
    user_scores = load_json_file(SCORES_FILE, {})
    user_best = user_scores.get(current_user, {}).get('best_score', 0)
    
    # En-têtes HTTP
    print("Content-Type: text/html; charset=utf-8")
    print()
    
    # Page HTML
    html = f"""<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🐍 Snake Game - {current_user}</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
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
            max-width: 900px;
            width: 100%;
            padding: 30px;
            animation: fadeIn 0.5s ease-in;
        }}
        
        @keyframes fadeIn {{
            from {{ opacity: 0; transform: translateY(-20px); }}
            to {{ opacity: 1; transform: translateY(0); }}
        }}
        
        .header {{
            text-align: center;
            margin-bottom: 20px;
        }}
        
        h1 {{
            color: #1e3c72;
            font-size: 2.5em;
            margin-bottom: 10px;
        }}
        
        .user-info {{
            background: linear-gradient(135deg, #1e3c7215 0%, #2a529815 100%);
            padding: 15px;
            border-radius: 10px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            flex-wrap: wrap;
            gap: 10px;
        }}
        
        .user-name {{
            font-weight: bold;
            color: #1e3c72;
            font-size: 1.2em;
        }}
        
        .user-best {{
            background: #ffd700;
            padding: 8px 15px;
            border-radius: 20px;
            font-weight: bold;
            color: #333;
        }}
        
        .game-container {{
            display: flex;
            gap: 20px;
            flex-wrap: wrap;
            justify-content: center;
        }}
        
        .game-area {{
            flex: 1;
            min-width: 300px;
        }}
        
        .canvas-container {{
            position: relative;
            background: #000;
            border-radius: 10px;
            overflow: hidden;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
            margin-bottom: 20px;
        }}
        
        #gameCanvas {{
            display: block;
            width: 100%;
            height: auto;
        }}
        
        .game-overlay {{
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.8);
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            color: white;
            transition: opacity 0.3s ease;
        }}
        
        .game-overlay.hidden {{
            opacity: 0;
            pointer-events: none;
        }}
        
        .overlay-title {{
            font-size: 3em;
            margin-bottom: 20px;
            text-shadow: 0 0 20px rgba(255, 255, 255, 0.5);
        }}
        
        .overlay-message {{
            font-size: 1.5em;
            margin-bottom: 30px;
        }}
        
        .controls {{
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
        }}
        
        .score-display {{
            display: flex;
            justify-content: space-around;
            margin-bottom: 15px;
            gap: 10px;
        }}
        
        .score-item {{
            text-align: center;
            flex: 1;
        }}
        
        .score-label {{
            color: #666;
            font-size: 0.9em;
            margin-bottom: 5px;
        }}
        
        .score-value {{
            font-size: 2em;
            font-weight: bold;
            color: #1e3c72;
        }}
        
        .buttons {{
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
        }}
        
        .btn {{
            flex: 1;
            min-width: 100px;
            padding: 12px 20px;
            border: none;
            border-radius: 8px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
        }}
        
        .btn-primary {{
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: white;
            box-shadow: 0 4px 15px rgba(30, 60, 114, 0.4);
        }}
        
        .btn-primary:hover {{
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(30, 60, 114, 0.6);
        }}
        
        .btn-secondary {{
            background: #6c757d;
            color: white;
        }}
        
        .btn-secondary:hover {{
            background: #5a6268;
        }}
        
        .sidebar {{
            width: 280px;
        }}
        
        .leaderboard {{
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
        }}
        
        .leaderboard h3 {{
            color: #1e3c72;
            margin-bottom: 15px;
            text-align: center;
            font-size: 1.3em;
        }}
        
        .leaderboard-item {{
            background: white;
            padding: 10px;
            margin: 8px 0;
            border-radius: 8px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
        }}
        
        .leaderboard-item.current-user {{
            background: linear-gradient(135deg, #ffd70015 0%, #ffa50015 100%);
            border: 2px solid #ffd700;
        }}
        
        .rank {{
            font-weight: bold;
            color: #1e3c72;
            width: 30px;
        }}
        
        .rank.gold {{ color: #ffd700; }}
        .rank.silver {{ color: #c0c0c0; }}
        .rank.bronze {{ color: #cd7f32; }}
        
        .player-name {{
            flex: 1;
            padding: 0 10px;
        }}
        
        .player-score {{
            font-weight: bold;
            color: #2a5298;
        }}
        
        .instructions {{
            background: #e7f3ff;
            border-left: 4px solid #2196F3;
            padding: 15px;
            border-radius: 5px;
        }}
        
        .instructions h4 {{
            color: #2196F3;
            margin-bottom: 10px;
        }}
        
        .instructions ul {{
            list-style: none;
            padding: 0;
        }}
        
        .instructions li {{
            padding: 5px 0;
            color: #555;
        }}
        
        .instructions li::before {{
            content: "→ ";
            color: #2196F3;
            font-weight: bold;
        }}
        
        .nav-link {{
            display: block;
            text-align: center;
            padding: 12px;
            background: #6c757d;
            color: white;
            text-decoration: none;
            border-radius: 8px;
            margin-top: 10px;
            transition: all 0.3s ease;
        }}
        
        .nav-link:hover {{
            background: #5a6268;
            transform: translateY(-2px);
        }}
        
        @media (max-width: 768px) {{
            .game-container {{
                flex-direction: column;
            }}
            
            .sidebar {{
                width: 100%;
            }}
            
            .overlay-title {{
                font-size: 2em;
            }}
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🐍 Snake Game</h1>
        </div>
        
        <div class="user-info">
            <span class="user-name">👤 {current_user}</span>
            <span class="user-best">🏆 Meilleur Score: {user_best}</span>
        </div>
        
        <div class="game-container">
            <div class="game-area">
                <div class="canvas-container">
                    <canvas id="gameCanvas" width="400" height="400"></canvas>
                    <div id="gameOverlay" class="game-overlay">
                        <div class="overlay-title">🐍 SNAKE</div>
                        <div class="overlay-message">Appuyez sur START pour jouer</div>
                        <button class="btn btn-primary" onclick="startGame()">▶️ START</button>
                    </div>
                </div>
                
                <div class="controls">
                    <div class="score-display">
                        <div class="score-item">
                            <div class="score-label">Score</div>
                            <div class="score-value" id="currentScore">0</div>
                        </div>
                        <div class="score-item">
                            <div class="score-label">Longueur</div>
                            <div class="score-value" id="snakeLength">1</div>
                        </div>
                    </div>
                    
                    <div class="buttons">
                        <button class="btn btn-primary" onclick="startGame()">🎮 Nouvelle Partie</button>
                        <button class="btn btn-secondary" onclick="togglePause()">⏸️ Pause</button>
                    </div>
                </div>
            </div>
            
            <div class="sidebar">
                <div class="leaderboard">
                    <h3>🏆 Classement</h3>
"""
    
    for i, entry in enumerate(leaderboard, 1):
        rank_class = 'gold' if i == 1 else ('silver' if i == 2 else ('bronze' if i == 3 else ''))
        is_current = 'current-user' if entry['username'] == current_user else ''
        html += f"""
                    <div class="leaderboard-item {is_current}">
                        <span class="rank {rank_class}">#{i}</span>
                        <span class="player-name">{entry['username']}</span>
                        <span class="player-score">{entry['score']}</span>
                    </div>
"""
    
    if not leaderboard:
        html += """
                    <div style="text-align: center; color: #999; padding: 20px;">
                        Aucun score enregistré
                    </div>
"""
    
    html += f"""
                </div>
                
                <div class="instructions">
                    <h4>📖 Instructions</h4>
                    <ul>
                        <li>Utilisez les flèches ⬆️⬇️⬅️➡️</li>
                        <li>Mangez les pommes 🍎</li>
                        <li>Évitez les murs et vous-même</li>
                        <li>Score = Longueur × 10</li>
                    </ul>
                </div>
                
                <a href="/cgi-bin/auth.py" class="nav-link">🏠 Retour au Dashboard</a>
                <a href="/cgi-bin/cookie_test.py" class="nav-link">🍪 Cookies</a>
            </div>
        </div>
    </div>
    
    <script>
        const canvas = document.getElementById('gameCanvas');
        const ctx = canvas.getContext('2d');
        const overlay = document.getElementById('gameOverlay');
        
        const gridSize = 20;
        const tileCount = canvas.width / gridSize;
        
        let snake = [{{x: 10, y: 10}}];
        let food = {{x: 15, y: 15}};
        let dx = 0;
        let dy = 0;
        let score = 0;
        let gameLoop = null;
        let isPaused = false;
        let gameSpeed = 100;
        
        function drawGame() {{
            // Clear canvas
            ctx.fillStyle = '#000';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            // Draw grid
            ctx.strokeStyle = '#111';
            ctx.lineWidth = 1;
            for (let i = 0; i <= tileCount; i++) {{
                ctx.beginPath();
                ctx.moveTo(i * gridSize, 0);
                ctx.lineTo(i * gridSize, canvas.height);
                ctx.stroke();
                
                ctx.beginPath();
                ctx.moveTo(0, i * gridSize);
                ctx.lineTo(canvas.width, i * gridSize);
                ctx.stroke();
            }}
            
            // Draw food
            ctx.fillStyle = '#ff4444';
            ctx.shadowBlur = 20;
            ctx.shadowColor = '#ff4444';
            ctx.beginPath();
            ctx.arc(
                food.x * gridSize + gridSize / 2,
                food.y * gridSize + gridSize / 2,
                gridSize / 2 - 2,
                0,
                Math.PI * 2
            );
            ctx.fill();
            ctx.shadowBlur = 0;
            
            // Draw snake
            snake.forEach((segment, index) => {{
                const gradient = ctx.createLinearGradient(
                    segment.x * gridSize,
                    segment.y * gridSize,
                    segment.x * gridSize + gridSize,
                    segment.y * gridSize + gridSize
                );
                
                if (index === 0) {{
                    // Head
                    gradient.addColorStop(0, '#00ff00');
                    gradient.addColorStop(1, '#00cc00');
                    ctx.fillStyle = gradient;
                    ctx.shadowBlur = 15;
                    ctx.shadowColor = '#00ff00';
                }} else {{
                    // Body
                    gradient.addColorStop(0, '#44ff44');
                    gradient.addColorStop(1, '#22cc22');
                    ctx.fillStyle = gradient;
                    ctx.shadowBlur = 5;
                    ctx.shadowColor = '#44ff44';
                }}
                
                ctx.fillRect(
                    segment.x * gridSize + 1,
                    segment.y * gridSize + 1,
                    gridSize - 2,
                    gridSize - 2
                );
            }});
            ctx.shadowBlur = 0;
        }}
        
        function update() {{
            if (isPaused) return;
            
            const head = {{x: snake[0].x + dx, y: snake[0].y + dy}};
            
            // Check wall collision
            if (head.x < 0 || head.x >= tileCount || head.y < 0 || head.y >= tileCount) {{
                gameOver();
                return;
            }}
            
            // Check self collision
            for (let segment of snake) {{
                if (head.x === segment.x && head.y === segment.y) {{
                    gameOver();
                    return;
                }}
            }}
            
            snake.unshift(head);
            
            // Check food collision
            if (head.x === food.x && head.y === food.y) {{
                score += 10;
                updateScore();
                spawnFood();
                
                // Increase speed slightly
                if (gameSpeed > 50) {{
                    gameSpeed -= 2;
                    clearInterval(gameLoop);
                    gameLoop = setInterval(gameStep, gameSpeed);
                }}
            }} else {{
                snake.pop();
            }}
        }}
        
        function gameStep() {{
            update();
            drawGame();
        }}
        
        function spawnFood() {{
            food = {{
                x: Math.floor(Math.random() * tileCount),
                y: Math.floor(Math.random() * tileCount)
            }};
            
            // Make sure food doesn't spawn on snake
            for (let segment of snake) {{
                if (food.x === segment.x && food.y === segment.y) {{
                    spawnFood();
                    return;
                }}
            }}
        }}
        
        function updateScore() {{
            document.getElementById('currentScore').textContent = score;
            document.getElementById('snakeLength').textContent = snake.length;
        }}
        
        function startGame() {{
            snake = [{{x: 10, y: 10}}];
            dx = 1;
            dy = 0;
            score = 0;
            gameSpeed = 100;
            isPaused = false;
            
            spawnFood();
            updateScore();
            
            overlay.classList.add('hidden');
            
            if (gameLoop) clearInterval(gameLoop);
            gameLoop = setInterval(gameStep, gameSpeed);
        }}
        
        function togglePause() {{
            isPaused = !isPaused;
        }}
        
        function gameOver() {{
            clearInterval(gameLoop);
            
            // Save score
            saveScore(score);
            
            overlay.querySelector('.overlay-title').textContent = '💀 GAME OVER';
            overlay.querySelector('.overlay-message').textContent = `Score Final: ${{score}}`;
            overlay.classList.remove('hidden');
        }}
        
        function saveScore(score) {{
            const formData = new FormData();
            formData.append('action', 'save_score');
            formData.append('score', score);
            
            fetch('/cgi-bin/snake.py', {{
                method: 'POST',
                body: new URLSearchParams(formData)
            }}).then(() => {{
                // Reload to update leaderboard
                setTimeout(() => location.reload(), 2000);
            }});
        }}
        
        // Keyboard controls
        document.addEventListener('keydown', (e) => {{
            switch(e.key) {{
                case 'ArrowUp':
                    if (dy === 0) {{ dx = 0; dy = -1; }}
                    e.preventDefault();
                    break;
                case 'ArrowDown':
                    if (dy === 0) {{ dx = 0; dy = 1; }}
                    e.preventDefault();
                    break;
                case 'ArrowLeft':
                    if (dx === 0) {{ dx = -1; dy = 0; }}
                    e.preventDefault();
                    break;
                case 'ArrowRight':
                    if (dx === 0) {{ dx = 1; dy = 0; }}
                    e.preventDefault();
                    break;
                case ' ':
                    togglePause();
                    e.preventDefault();
                    break;
            }}
        }});
        
        // Initial draw
        drawGame();
    </script>
</body>
</html>"""
    
    print(html)

if __name__ == "__main__":
    main()
