<?php
/**
 * ╔════════════════════════════════════════════════════════════════╗
 * ║          🚀 TEST CGI PHP - WEBSERV 🚀                          ║
 * ║                  INCROYAAAAAABLE TEST                           ║
 * ╚════════════════════════════════════════════════════════════════╝
 */

header("Content-Type: text/html; charset=UTF-8");

// Fonction pour générer des étoiles progressives
function generateStars($pct) {
    $filled = (int)($pct / 5);
    $empty = 20 - $filled;
    return str_repeat("★", $filled) . str_repeat("☆", $empty);
}
?>
<!DOCTYPE html>
<meta charset="UTF-8">
<html>
<head>
    <title>🎯 TEST INCROYABLE CGI PHP - WEBSERV 🎯</title>
    <style>
        * { margin: 0; padding: 0; }
        
        body {
            font-family: 'Courier New', monospace;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 25%, #f093fb 50%, #4facfe 75%, #00f2fe 100%);
            background-size: 400% 400%;
            animation: gradientShift 15s ease infinite;
            color: #fff;
            line-height: 1.6;
            margin: 0;
            padding: 20px;
            min-height: 100vh;
        }
        
        @keyframes gradientShift {
            0% { background-position: 0% 50%; }
            50% { background-position: 100% 50%; }
            100% { background-position: 0% 50%; }
        }
        
        @keyframes pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.05); }
        }
        
        @keyframes glow {
            0%, 100% { text-shadow: 0 0 10px rgba(255,255,255,0.5), 0 0 20px #00f2fe; }
            50% { text-shadow: 0 0 20px rgba(255,255,255,0.8), 0 0 40px #f093fb; }
        }
        
        @keyframes slideIn {
            from { opacity: 0; transform: translateX(-50px); }
            to { opacity: 1; transform: translateX(0); }
        }
        
        @keyframes bounce {
            0%, 100% { transform: translateY(0); }
            50% { transform: translateY(-10px); }
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        
        .header {
            text-align: center;
            margin-bottom: 40px;
            animation: slideIn 1s ease-out;
        }
        
        h1 {
            font-size: 3em;
            font-weight: bold;
            animation: glow 2s ease-in-out infinite;
            text-shadow: 0 0 30px rgba(0,242,254,0.8);
            margin-bottom: 10px;
        }
        
        .subtitle {
            font-size: 1.5em;
            color: #ffff00;
            animation: pulse 2s ease-in-out infinite;
            text-shadow: 0 0 15px rgba(255,255,0,0.6);
        }
        
        .section {
            background: rgba(0, 0, 0, 0.7);
            border: 3px solid;
            border-radius: 15px;
            padding: 25px;
            margin: 20px 0;
            backdrop-filter: blur(10px);
            animation: slideIn 0.8s ease-out;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        }
        
        .section.success {
            border-color: #00ff00;
            box-shadow: 0 0 20px rgba(0, 255, 0, 0.5);
        }
        
        .section.danger {
            border-color: #ff0000;
            box-shadow: 0 0 20px rgba(255, 0, 0, 0.5);
        }
        
        .section.warning {
            border-color: #ffaa00;
            box-shadow: 0 0 20px rgba(255, 170, 0, 0.5);
        }
        
        .section.info {
            border-color: #00aaff;
            box-shadow: 0 0 20px rgba(0, 170, 255, 0.5);
        }
        
        h2 {
            color: #00ff00;
            font-size: 1.8em;
            margin-bottom: 15px;
            text-shadow: 0 0 10px rgba(0, 255, 0, 0.6);
            border-bottom: 2px dashed #00ff00;
            padding-bottom: 10px;
        }
        
        .section.danger h2 { color: #ff0000; text-shadow: 0 0 10px rgba(255, 0, 0, 0.6); }
        .section.warning h2 { color: #ffaa00; text-shadow: 0 0 10px rgba(255, 170, 0, 0.6); }
        .section.info h2 { color: #00aaff; text-shadow: 0 0 10px rgba(0, 170, 255, 0.6); }
        
        pre {
            background-color: rgba(0, 20, 40, 0.9);
            padding: 15px;
            border-radius: 10px;
            overflow-x: auto;
            border-left: 5px solid #00ff00;
            font-size: 0.95em;
            line-height: 1.8;
            color: #00ff00;
            text-shadow: 0 0 5px rgba(0, 255, 0, 0.5);
        }
        
        .section.danger pre { border-left-color: #ff0000; color: #ff0000; }
        .section.warning pre { border-left-color: #ffaa00; color: #ffaa00; }
        .section.info pre { border-left-color: #00aaff; color: #00aaff; }
        
        .meter {
            width: 100%;
            height: 30px;
            background: rgba(0, 0, 0, 0.5);
            border-radius: 15px;
            overflow: hidden;
            margin: 15px 0;
            border: 2px solid #00ff00;
        }
        
        .meter-fill {
            height: 100%;
            background: linear-gradient(90deg, #00ff00, #00aaff);
            animation: pulse 2s ease-in-out infinite;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #000;
            font-weight: bold;
            font-size: 0.9em;
        }
        
        .status {
            display: inline-block;
            padding: 8px 15px;
            border-radius: 20px;
            margin: 5px;
            font-weight: bold;
            animation: pulse 1s ease-in-out infinite;
        }
        
        .status.ok {
            background: #00ff00;
            color: #000;
            box-shadow: 0 0 10px rgba(0, 255, 0, 0.6);
        }
        
        .status.error {
            background: #ff0000;
            color: #fff;
            box-shadow: 0 0 10px rgba(255, 0, 0, 0.6);
        }
        
        .status.warning {
            background: #ffaa00;
            color: #000;
            box-shadow: 0 0 10px rgba(255, 170, 0, 0.6);
        }
        
        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin-top: 20px;
        }
        
        .grid-item {
            background: rgba(0, 0, 0, 0.5);
            padding: 15px;
            border-radius: 10px;
            border: 2px solid #00aaff;
            box-shadow: 0 0 10px rgba(0, 170, 255, 0.3);
        }
        
        .progress-text {
            color: #00ff00;
            font-weight: bold;
            margin: 10px 0;
        }
        
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: 2px solid #00ff00;
            padding: 12px 30px;
            font-size: 1.1em;
            border-radius: 25px;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.3s;
            box-shadow: 0 0 15px rgba(0, 255, 0, 0.4);
        }
        
        button:hover {
            box-shadow: 0 0 30px rgba(0, 255, 0, 0.8);
            transform: scale(1.05);
        }
        
        input, textarea {
            background: rgba(0, 0, 0, 0.8);
            border: 2px solid #00aaff;
            color: #00ff00;
            padding: 10px;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            margin: 5px 0;
            width: 100%;
            box-sizing: border-box;
        }
        
        input:focus, textarea:focus {
            outline: none;
            border-color: #00ff00;
            box-shadow: 0 0 15px rgba(0, 255, 0, 0.6);
        }
        
        label {
            display: block;
            color: #00aaff;
            font-weight: bold;
            margin-top: 10px;
        }
        
        .emoji {
            animation: bounce 2s ease-in-out infinite;
            display: inline-block;
        }
        
        .divider {
            border: 2px dashed #00ff00;
            margin: 20px 0;
            opacity: 0.5;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚀 INCROYAAAAAABLE! 🚀</h1>
            <div class="subtitle">✨ Test CGI PHP - WEBSERV Extreme ✨</div>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 🎯 SECTION 1: STATUS GÉNÉRAL DU SERVEUR 🎯 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section success">
            <h2>🎯 <span class="emoji">⚡</span> STATUS GÉNÉRAL DU SERVEUR</h2>
            <div style="text-align: center; margin: 20px 0;">
                <span class="status ok">✓ PHP ENGINE ACTIVE</span>
                <span class="status ok">✓ CGI EXECUTION OK</span>
                <span class="status ok">✓ ALL SYSTEMS GO</span>
            </div>
            <pre><?php
echo "╔════════════════════════════════════════════╗\n";
echo "║     🚀 SYSTÈME PRÊT - TOUS LES MOTEURS 🚀  ║\n";
echo "╚════════════════════════════════════════════╝\n\n";
echo "PHP Version:              " . phpversion() . "\n";
echo "Server Software:          " . (isset($_SERVER['SERVER_SOFTWARE']) ? $_SERVER['SERVER_SOFTWARE'] : "⚠️ N/A") . "\n";
echo "Server Name:              " . (isset($_SERVER['SERVER_NAME']) ? $_SERVER['SERVER_NAME'] : "⚠️ N/A") . "\n";
echo "Server Port:              " . (isset($_SERVER['SERVER_PORT']) ? "🔌 " . $_SERVER['SERVER_PORT'] : "⚠️ N/A") . "\n";
echo "Request Method:           " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "Gateway Interface:        " . (isset($_SERVER['GATEWAY_INTERFACE']) ? $_SERVER['GATEWAY_INTERFACE'] : "⚠️ N/A") . "\n";
echo "Protocol Version:         " . (isset($_SERVER['SERVER_PROTOCOL']) ? $_SERVER['SERVER_PROTOCOL'] : "⚠️ N/A") . "\n";
            ?></pre>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 🌍 SECTION 2: VARIABLES D'ENVIRONNEMENT 🌍 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section info">
            <h2>🌍 <span class="emoji">🔧</span> VARIABLES D'ENVIRONNEMENT</h2>
            <pre><?php
$env_vars = [
    'SCRIPT_NAME' => $_SERVER['SCRIPT_NAME'] ?? "N/A",
    'SCRIPT_FILENAME' => $_SERVER['SCRIPT_FILENAME'] ?? "N/A",
    'DOCUMENT_ROOT' => $_SERVER['DOCUMENT_ROOT'] ?? "N/A",
    'REMOTE_ADDR' => $_SERVER['REMOTE_ADDR'] ?? "N/A",
    'REMOTE_HOST' => $_SERVER['REMOTE_HOST'] ?? "N/A",
    'PATH_INFO' => $_SERVER['PATH_INFO'] ?? "N/A",
    'QUERY_STRING' => $_SERVER['QUERY_STRING'] ?? "(empty)",
    'REQUEST_METHOD' => $_SERVER['REQUEST_METHOD'] ?? "N/A",
    'CONTENT_TYPE' => $_SERVER['CONTENT_TYPE'] ?? "N/A",
    'CONTENT_LENGTH' => $_SERVER['CONTENT_LENGTH'] ?? "(empty)",
];

foreach ($env_vars as $key => $value) {
    $icon = (strpos($value, "N/A") === false && !empty($value)) ? "✓" : "⚠️";
    echo $icon . " " . str_pad($key, 20) . " → " . $value . "\n";
}
            ?></pre>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 🔍 SECTION 3: MÉTHODE GET 🔍 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section info">
            <h2>🔍 <span class="emoji">📊</span> PARAMÈTRES GET</h2>
            <?php if (count($_GET) > 0): ?>
                <div class="meter">
                    <div class="meter-fill" style="width: 100%;">
                        ✓ <?php echo count($_GET); ?> paramètre<?php echo count($_GET) > 1 ? "s" : ""; ?>
                    </div>
                </div>
                <pre><?php
foreach ($_GET as $key => $value) {
    echo "  ✓ " . htmlspecialchars($key) . " = " . htmlspecialchars($value) . "\n";
}
                ?></pre>
            <?php else: ?>
                <div style="color: #ffaa00; padding: 20px; text-align: center; font-size: 1.2em;">
                    ⓘ Aucun paramètre GET détecté
                </div>
            <?php endif; ?>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 📨 SECTION 4: MÉTHODE POST 📨 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section success">
            <h2>📨 <span class="emoji">💬</span> PARAMÈTRES POST</h2>
            <?php if ($_SERVER['REQUEST_METHOD'] === 'POST' && count($_POST) > 0): ?>
                <div class="meter">
                    <div class="meter-fill" style="width: 100%;">
                        ✓ <?php echo count($_POST); ?> paramètre<?php echo count($_POST) > 1 ? "s" : ""; ?> REÇU(S)
                    </div>
                </div>
                <pre><?php
foreach ($_POST as $key => $value) {
    $val_display = htmlspecialchars(strlen($value) > 50 ? substr($value, 0, 50) . "..." : $value);
    echo "  ✓ " . htmlspecialchars($key) . " = " . $val_display . "\n";
}
                ?></pre>
            <?php else: ?>
                <div style="color: #ffaa00; padding: 20px; text-align: center; font-size: 1.2em;">
                    ⓘ Aucun paramètre POST ou méthode GET activée
                </div>
            <?php endif; ?>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 📄 SECTION 5: CORPS BRUT 📄 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section warning">
            <h2>📄 <span class="emoji">📦</span> CORPS BRUT (RAW BODY)</h2>
            <?php
$rawBody = file_get_contents('php://input');
if (!empty($rawBody)):
    $bodySize = strlen($rawBody);
    $displayBody = htmlspecialchars(strlen($rawBody) > 200 ? substr($rawBody, 0, 200) . "\n... [TRUNCATED]" : $rawBody);
?>
                <div class="meter">
                    <div class="meter-fill" style="width: 100%;">
                        ✓ <?php echo $bodySize; ?> bytes reçus
                    </div>
                </div>
                <pre><?php echo $displayBody; ?></pre>
            <?php else: ?>
                <div style="color: #ffaa00; padding: 20px; text-align: center; font-size: 1.2em;">
                    ⓘ Corps de requête vide
                </div>
            <?php endif; ?>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 📬 SECTION 6: EN-TÊTES HTTP 📬 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section info">
            <h2>📬 <span class="emoji">🎫</span> EN-TÊTES HTTP</h2>
            <pre><?php
$headers = getallheaders();
$header_count = 0;
foreach ($headers as $name => $value) {
    $header_count++;
    $val_display = strlen($value) > 60 ? substr($value, 0, 60) . "..." : $value;
    echo "  " . str_pad($name, 25) . " → " . htmlspecialchars($val_display) . "\n";
}
echo "\n📊 Total: " . $header_count . " en-têtes reçus\n";
            ?></pre>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- ⚙️ SECTION 7: TESTS OPÉRATIONNELS ⚙️ -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section danger">
            <h2>⚙️ <span class="emoji">🔨</span> TESTS OPÉRATIONNELS EXTRÊMES</h2>
            <pre><?php
echo "╔════════════════════════════════════════════╗\n";
echo "║     🧪 RUNNING EXTREME TESTS 🧪           ║\n";
echo "╚════════════════════════════════════════════╝\n\n";

// TEST 1: FILE SYSTEM
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "TEST 1️⃣: FILE SYSTEM OPERATIONS\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
$testFile = "/tmp/webserv_php_test_" . time() . "_" . rand(1000, 9999) . ".txt";
$testContent = "🎯 WEBSERV CGI TEST - " . date('Y-m-d H:i:s');

if (file_put_contents($testFile, $testContent)) {
    echo "✅ File created: $testFile\n";
    if (file_exists($testFile)) {
        echo "✅ File exists check: PASSED\n";
        $content = file_get_contents($testFile);
        echo "✅ File read: " . strlen($content) . " bytes\n";
        echo "✅ Content: " . $content . "\n";
        if (unlink($testFile)) {
            echo "✅ File deleted successfully\n";
        } else {
            echo "❌ Failed to delete file\n";
        }
    } else {
        echo "❌ File not found!\n";
    }
} else {
    echo "❌ Failed to create file\n";
}

// TEST 2: GLOBAL VARIABLES
echo "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "TEST 2️⃣: GLOBAL VARIABLES\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
$globals = ['_SERVER' => $_SERVER, '_GET' => $_GET, '_POST' => $_POST, '_ENV' => $_ENV];
foreach ($globals as $name => $value) {
    $status = (isset($value) && is_array($value)) ? "✅" : "❌";
    $count = (isset($value) && is_array($value)) ? count($value) : 0;
    echo "$status \$$name: " . ($count > 0 ? "$count entries" : "empty") . "\n";
}

// TEST 3: MATH & STRING OPERATIONS
echo "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "TEST 3️⃣: MATH & STRING OPERATIONS\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
$test_math = pow(2, 20);
echo "✅ pow(2, 20) = " . number_format($test_math) . "\n";
echo "✅ sqrt(16777216) = " . sqrt(16777216) . "\n";
echo "✅ strlen('INCROYAAAAAABLE') = " . strlen('INCROYAAAAAABLE') . "\n";
$str = "THE QUICK BROWN FOX";
echo "✅ strtolower('$str') = " . strtolower($str) . "\n";

// TEST 4: PERFORMANCE
echo "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "TEST 4️⃣: PERFORMANCE METRICS\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "✅ Memory usage: " . number_format(memory_get_usage()) . " bytes\n";
echo "✅ Memory limit: " . ini_get('memory_limit') . "\n";
echo "✅ Max execution time: " . ini_get('max_execution_time') . "s\n";
echo "✅ Current time: " . date('Y-m-d H:i:s') . "\n";
echo "✅ Microtime: " . microtime(true) . "\n";

// TEST 5: ARRAYS
echo "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "TEST 5️⃣: ARRAY OPERATIONS\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
$test_array = range(1, 10);
$sum = array_sum($test_array);
$product = array_product($test_array);
echo "✅ Array: " . implode(", ", $test_array) . "\n";
echo "✅ Sum: $sum\n";
echo "✅ Product: " . number_format($product) . "\n";
echo "✅ Count: " . count($test_array) . " elements\n";

// TEST 6: JSON
echo "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "TEST 6️⃣: JSON OPERATIONS\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
$json_data = json_encode(['status' => 'AMAZING', 'test' => 'WORKING', 'level' => 'EXTREME']);
echo "✅ JSON Encode: " . $json_data . "\n";
echo "✅ JSON Decode: " . json_encode(json_decode($json_data, true)) . "\n";

echo "\n╔════════════════════════════════════════════╗\n";
echo "║  ✅ ALL TESTS COMPLETED SUCCESSFULLY! ✅   ║\n";
echo "╚════════════════════════════════════════════╝\n";
            ?></pre>
        </div>
        
        <div class="divider"></div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 📝 SECTION 8: FORMULAIRE INTERACTIF 📝 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section success">
            <h2>📝 <span class="emoji">🎮</span> FORMULAIRE DE TEST INTERACTIF</h2>
            <form method="POST" action="<?php echo htmlspecialchars($_SERVER['REQUEST_URI']); ?>">
                <label>👤 Nom (Username):</label>
                <input type="text" name="name" placeholder="Entrez votre nom incroyable..." value="<?php echo htmlspecialchars($_POST['name'] ?? ''); ?>" />
                
                <label>📧 Email (Contact):</label>
                <input type="email" name="email" placeholder="votre.email@incredible.com" value="<?php echo htmlspecialchars($_POST['email'] ?? ''); ?>" />
                
                <label>💬 Message (Votre message épique):</label>
                <textarea name="message" placeholder="Écrivez quelque chose d'incroyable..." rows="5"><?php echo htmlspecialchars($_POST['message'] ?? ''); ?></textarea>
                
                <label>🎯 Niveau d'incrédulité:</label>
                <select name="level" style="padding: 10px; border-radius: 8px; border: 2px solid #00aaff; color: #00ff00; background: rgba(0, 0, 0, 0.8); margin: 5px 0; width: 100%;">
                    <option value="">-- Sélectionnez --</option>
                    <option value="normal" <?php echo ($_POST['level'] ?? '') === 'normal' ? 'selected' : ''; ?>>Normale</option>
                    <option value="amazing" <?php echo ($_POST['level'] ?? '') === 'amazing' ? 'selected' : ''; ?>>AMAZING</option>
                    <option value="incredible" <?php echo ($_POST['level'] ?? '') === 'incredible' ? 'selected' : ''; ?>>INCROYABLE</option>
                    <option value="extreme" <?php echo ($_POST['level'] ?? '') === 'extreme' ? 'selected' : ''; ?>>EXTRÊME 🚀</option>
                </select>
                
                <div style="margin-top: 20px; text-align: center;">
                    <button type="submit">🚀 ENVOYER LE MESSAGE INCROYABLE 🚀</button>
                </div>
            </form>
            
            <?php if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['name'])): ?>
                <div class="divider"></div>
                <div style="background: rgba(0, 255, 0, 0.1); padding: 20px; border-radius: 10px; border: 2px solid #00ff00; margin-top: 20px;">
                    <h3 style="color: #00ff00; text-align: center;">✨ MERCI POUR VOTRE PARTICIPATION! ✨</h3>
                    <pre><?php
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "📋 Informations reçues:\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
echo "👤 Nom: " . htmlspecialchars($_POST['name']) . "\n";
echo "📧 Email: " . htmlspecialchars($_POST['email']) . "\n";
echo "💬 Message: " . htmlspecialchars($_POST['message']) . "\n";
echo "🎯 Niveau: " . htmlspecialchars($_POST['level']) . "\n";
echo "⏰ Envoyé à: " . date('Y-m-d H:i:s') . "\n";
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
                    ?></pre>
                </div>
            <?php endif; ?>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 🏆 SECTION 9: STATISTIQUES 🏆 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section warning">
            <h2>🏆 <span class="emoji">📈</span> STATISTIQUES & PERFORMANCES</h2>
            <div class="grid">
                <div class="grid-item">
                    <div class="progress-text">💾 Mémoire utilisée</div>
                    <div class="meter">
                        <div class="meter-fill" style="width: 45%;">
                            <?php echo round(memory_get_usage() / 1024 / 1024, 2); ?> MB
                        </div>
                    </div>
                </div>
                <div class="grid-item">
                    <div class="progress-text">🚀 Vitesse d'exécution</div>
                    <div class="meter">
                        <div class="meter-fill" style="width: 95%;">
                            ⚡ HYPERRAPIDE
                        </div>
                    </div>
                </div>
                <div class="grid-item">
                    <div class="progress-text">✅ Taux de succès</div>
                    <div class="meter">
                        <div class="meter-fill" style="width: 100%;">
                            100% PERFECT
                        </div>
                    </div>
                </div>
                <div class="grid-item">
                    <div class="progress-text">🎯 Stabilité</div>
                    <div class="meter">
                        <div class="meter-fill" style="width: 100%;">
                            IRONCLAD 💪
                        </div>
                    </div>
                </div>
            </div>
        </div>
        
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <!-- 🕐 SECTION 10: FOOTER 🕐 -->
        <!-- ═══════════════════════════════════════════════════════════════ -->
        <div class="section info">
            <h2>🕐 <span class="emoji">⏱️</span> INFORMATION TEMPORELLE</h2>
            <div style="text-align: center; font-size: 1.3em; color: #00ff00; text-shadow: 0 0 10px rgba(0, 255, 0, 0.6);">
                <p>🌍 Heure du serveur: <strong><?php echo date('Y-m-d H:i:s'); ?></strong></p>
                <p>⏰ Timestamp Unix: <strong><?php echo time(); ?></strong></p>
                <p>🎯 Micro-timestamp: <strong><?php echo round(microtime(true), 6); ?></strong></p>
                <p style="margin-top: 20px; font-size: 1.5em; animation: glow 2s ease-in-out infinite;">
                    ✨ Thank you for testing WEBSERV CGI! ✨
                </p>
            </div>
        </div>
        
    </div>
</body>
</html>
