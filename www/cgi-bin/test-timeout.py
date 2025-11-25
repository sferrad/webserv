#!/usr/bin/env python3

import time
import sys

# Headers CGI
print("Content-Type: text/html")
print()

# Début de la réponse
print("<!DOCTYPE html>")
print("<html><head><title>Test Timeout</title></head><body>")
print("<h1>Script en cours d'exécution...</h1>")
print("<p>Ce script va dormir pendant 15 secondes pour tester le timeout.</p>")
sys.stdout.flush()

# Simulation d'un traitement long (15 secondes, timeout configuré à 10 secondes)
time.sleep(35)

print("<p>Si vous voyez ceci, le timeout n'a pas fonctionné !</p>")
print("</body></html>")