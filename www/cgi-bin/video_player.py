#!/usr/bin/env python3
import os
import cgi
import sys
import urllib.parse
import cgitb

cgitb.enable()

# Directory where videos are stored (relative to the script execution)
VIDEO_DIR = "../videos"

def list_videos():
    videos = []
    if os.path.exists(VIDEO_DIR):
        try:
            for f in os.listdir(VIDEO_DIR):
                if f.lower().endswith(('.mp4', '.webm', '.ogg', '.mov')):
                    videos.append(f)
        except OSError:
            pass
    return videos

def get_mime_type(filename):
    ext = os.path.splitext(filename)[1].lower()
    if ext == '.mp4':
        return 'video/mp4'
    elif ext == '.webm':
        return 'video/webm'
    elif ext == '.ogg':
        return 'video/ogg'
    elif ext == '.mov':
        return 'video/quicktime'
    return 'video/mp4'

def save_uploaded_file():
    message = ""
    if os.environ.get('REQUEST_METHOD') == 'POST':
        try:
            form = cgi.FieldStorage()
            if 'video_file' in form:
                fileitem = form['video_file']
                if fileitem.filename:
                    # Secure the filename
                    fn = os.path.basename(fileitem.filename)
                    # Check extension
                    if fn.lower().endswith(('.mp4', '.webm', '.ogg', '.mov')):
                        try:
                            # Ensure directory exists
                            if not os.path.exists(VIDEO_DIR):
                                os.makedirs(VIDEO_DIR)
                                
                            with open(os.path.join(VIDEO_DIR, fn), 'wb') as f:
                                # Read in chunks to handle large files better
                                while True:
                                    chunk = fileitem.file.read(100000)
                                    if not chunk:
                                        break
                                    f.write(chunk)
                            message = f"<div style='color: #00ff9d; margin: 10px 0; padding: 10px; border: 1px solid #00ff9d; border-radius: 4px; background: rgba(0, 255, 157, 0.1);'>✅ Successfully uploaded: {fn}</div>"
                        except Exception as e:
                            message = f"<div style='color: #ff6b6b; margin: 10px 0; padding: 10px; border: 1px solid #ff6b6b; border-radius: 4px; background: rgba(255, 107, 107, 0.1);'>❌ Error saving file: {str(e)}</div>"
                    else:
                        message = "<div style='color: #ff6b6b; margin: 10px 0; padding: 10px; border: 1px solid #ff6b6b; border-radius: 4px; background: rgba(255, 107, 107, 0.1);'>❌ Invalid file type. Allowed: .mp4, .webm, .ogg, .mov</div>"
                else:
                    message = "<div style='color: #ff6b6b; margin: 10px 0; padding: 10px; border: 1px solid #ff6b6b; border-radius: 4px; background: rgba(255, 107, 107, 0.1);'>❌ No file selected</div>"
            else:
                message = "<div style='color: #ff6b6b; margin: 10px 0; padding: 10px; border: 1px solid #ff6b6b; border-radius: 4px; background: rgba(255, 107, 107, 0.1);'>❌ Form data missing video_file</div>"
        except Exception as e:
             message = f"<div style='color: #ff6b6b; margin: 10px 0; padding: 10px; border: 1px solid #ff6b6b; border-radius: 4px; background: rgba(255, 107, 107, 0.1);'>❌ Upload error: {str(e)}</div>"
    return message

def generate_html():
    # Handle upload first
    upload_message = save_uploaded_file()

    # Check if we have query parameters
    query_string = os.environ.get('QUERY_STRING', '')
    params = {}
    if query_string:
        for pair in query_string.split('&'):
            if '=' in pair:
                key, value = pair.split('=', 1)
                params[key] = urllib.parse.unquote(value)
    
    video_file = params.get('video')
    
    videos = list_videos()
    
    html = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CGI Video Player</title>
    <style>
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background-color: #1a1a1a; 
            color: #e0e0e0;
            padding: 20px; 
            margin: 0;
        }
        .container { 
            max-width: 900px; 
            margin: 0 auto; 
            background: #2d2d2d; 
            padding: 30px; 
            border-radius: 12px; 
            box-shadow: 0 4px 15px rgba(0,0,0,0.3); 
        }
        h1 { 
            color: #00ff9d; 
            text-align: center;
            margin-bottom: 30px;
            border-bottom: 2px solid #00ff9d;
            padding-bottom: 10px;
        }
        ul { list-style-type: none; padding: 0; }
        li { 
            margin: 10px 0; 
            background: #363636;
            border-radius: 6px;
            transition: transform 0.2s;
        }
        li:hover {
            transform: translateX(10px);
            background: #404040;
        }
        a { 
            display: block;
            text-decoration: none; 
            color: #fff; 
            font-size: 18px; 
            padding: 15px;
        }
        .player { 
            margin-top: 20px; 
            background: #000;
            padding: 10px;
            border-radius: 8px;
        }
        video { 
            width: 100%; 
            border-radius: 4px; 
            max-height: 70vh;
        }
        .back { 
            display: inline-block; 
            margin-top: 20px; 
            color: #00ff9d; 
            padding: 10px 20px;
            border: 1px solid #00ff9d;
            border-radius: 4px;
            transition: all 0.3s;
        }
        .back:hover {
            background: #00ff9d;
            color: #1a1a1a;
            text-decoration: none;
        }
        .video-title {
            color: #00ff9d;
            margin-bottom: 15px;
        }
        .no-videos {
            text-align: center;
            color: #888;
            padding: 20px;
        }
        .upload-section {
            background: #363636;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 30px;
            border: 1px solid #444;
        }
        .upload-btn {
            background: #00ff9d;
            color: #1a1a1a;
            border: none;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            font-size: 16px;
            transition: background 0.3s;
        }
        .upload-btn:hover {
            background: #00cc7d;
        }
        input[type="file"] {
            margin-bottom: 15px;
            color: #fff;
            width: 100%;
            padding: 10px;
            background: #222;
            border-radius: 4px;
            border: 1px solid #444;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎬 CGI Video Player</h1>
"""
    html += upload_message

    if video_file:
        # Security check: prevent directory traversal
        if os.path.basename(video_file) != video_file or video_file not in videos:
             html += "<p style='color:#ff6b6b; text-align: center;'>⚠️ Invalid video file or file not found.</p>"
             html += f'<div style="text-align: center;"><a href="video_player.py" class="back">← Back to list</a></div>'
        else:
            mime_type = get_mime_type(video_file)
            html += f"""
            <div class="player">
                <h2 class="video-title">Now Playing: {video_file}</h2>
                <video controls autoplay>
                    <source src="/videos/{video_file}" type="{mime_type}">
                    Your browser does not support the video tag.
                </video>
            </div>
            <div style="text-align: center;">
                <a href="video_player.py" class="back">← Back to list</a>
            </div>
            """
    else:
        html += """
        <div class="upload-section">
            <h2 style="margin-top: 0; color: #00ff9d;">📤 Upload Video</h2>
            <form action="video_player.py" method="post" enctype="multipart/form-data">
                <input type="file" name="video_file" accept=".mp4,.webm,.ogg,.mov">
                <br>
                <input type="submit" value="Upload Video" class="upload-btn">
            </form>
        </div>
        """
        
        html += "<h2>Available Videos</h2><ul>"
        if not videos:
            html += "<li class='no-videos'>No videos found in /videos directory.<br>Please add some .mp4 files to www/videos/</li>"
        for v in videos:
            html += f'<li><a href="?video={v}">🎥 {v}</a></li>'
        html += "</ul>"

    html += """
    </div>
</body>
</html>
"""
    return html

if __name__ == "__main__":
    print("Content-Type: text/html; charset=utf-8\r\n\r\n", end='')
    print(generate_html())
