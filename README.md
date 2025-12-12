# webserv

**webserv** est un projet du cursus **42** : implémenter un **serveur HTTP/1.1** en **C++98**.  
Le serveur lit une configuration de type *nginx-like*, gère plusieurs ports, sert des fichiers statiques, exécute des **CGI** (Python/PHP), supporte l’upload **PUT/POST**, l’**autoindex**, les redirections, et une gestion **native des cookies** côté C++.

---

## Build

Le binaire généré s’appelle **`webserv`** (voir `Makefile`).

```bash
make
```

Nettoyage :

```bash
make clean
make fclean
make re
```

---

## Run

Le serveur prend un fichier de configuration en argument.

Exemples de conf disponibles dans `conf_file/` :

- `conf_file/default.conf`
- `conf_file/webserv.conf`

Lancement :

```bash
./webserv conf_file/default.conf
# ou
./webserv conf_file/webserv.conf
```

---

## Configuration (conf_file)

### Exemple minimal (default.conf)

`conf_file/default.conf` lance un serveur sur **127.0.0.1:8080** avec :

- `root ./www`
- `index index.html`
- pages d’erreur: `/error/400.html`, `/error/403.html`, `/error/404.html`, `/error/405.html`, `/error/500.html`

### Configuration avancée (webserv.conf)

`conf_file/webserv.conf` définit plusieurs `server {}` (au moins **8080** et **8081**, et un autre server commence aussi sur **2525** dans le fichier).

On y retrouve aussi des `location` avec :

- `allowed_methods GET POST DELETE PUT`
- `client_max_body_size` (ex: `10M`, `120M`, ou une valeur petite pour test)
- `autoindex on/off`
- `upload_store` (ex: `./www/custom_uploads`, `./www/put_test`)
- `return 301` / `return 302` pour les redirections
- `cgi_pass` pour associer une extension à un exécutable (ex: `.py` → `/usr/bin/python3`, `.php` → `/usr/bin/php-cgi`)

---

## Fonctionnalités implémentées (repo)

### 1) Serveur non-bloquant & multi-ports
- Le serveur utilise **epoll** pour multiplexer les sockets (voir `srcs/Server.cpp`).
- Il écoute sur **tous les ports uniques** déclarés dans la conf, et sélectionne la conf correspondante selon le port/host.

### 2) Parsing HTTP + méthodes
Le handler (`HttpRequestHandler`) gère la logique HTTP et expose notamment :
- `GET` (fichiers statiques + index)
- `POST` (body classique + cas upload selon routes)
- `DELETE` (suppression autorisée sur certaines locations, ex: `/uploads` dans la conf)

### 3) Autoindex
Quand `autoindex on`, le serveur peut générer une page listant le contenu d’un répertoire (fonction `generateAutoindex(...)`).

### 4) Upload
La conf supporte `upload_store` (ex: `./www/custom_uploads`).  
Exemple dans `conf_file/webserv.conf` :
- `/uploads` accepte `POST DELETE` avec `upload_store ./www/custom_uploads`
- `/put_test` accepte `PUT` et stocke dans `./www/put_test`

### 5) CGI (Python / PHP)
CGI géré dans `srcs/CGI/HandleCGI.cpp` avec exécution via `execve`, création d’`env` CGI (`REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, etc.) et gestion des timeouts.

Dans la conf (ex: `conf_file/webserv.conf`) :
- `.py` → `/usr/bin/python3`
- `.php` → `/usr/bin/php-cgi`

Dossier CGI : `www/cgi-bin/`  
Scripts présents (exemples) :
- `auth.py` (login/register + sessions)
- `cookie_test.py` (gestion de cookies via CGI)
- `show_cookies.py` (affiche les cookies reçus côté serveur)
- `snake.py`, `video_player.py`, etc.

### 6) Cookies (C++ natif)
Le serveur parse l’en-tête `Cookie:` et peut renvoyer des headers `Set-Cookie:`.

Dans `HttpRequestHandler` :
- parsing des cookies (`parseCookies()`)
- API cookies :
  - `getCookie(name)`
  - `setCookie(name, value, maxAge, path)`
  - `deleteCookie(name)`
  - `hasCookie(name)`
- génération automatique des headers `Set-Cookie` via `getCookiesHeader()`

Des pages de démo existent dans `www/`, notamment :
- `www/test_cookies_cpp.html` (tests cookies côté navigateur)
- `www/cookie_doc.html` (doc/explications)
- `www/index.html` affiche aussi des infos basées sur cookies (ex: `visit_count`, `last_visit`)

---

## Pages statiques (www)

Le dossier `www/` contient vos pages HTML servies par le serveur, par exemple :
- `www/index.html` (page d’accueil)
- `www/port8081.html` (page dédiée au serveur 8081)
- `www/error/*` (pages d’erreur)
- `www/cgi-bin/index.html` (index de démonstration CGI)

---

## Équipe

- **sferrad**
- **Kaddouu** — https://github.com/Kaddouu
- **Bilal-El-Halimi** — https://github.com/Bilal-El-Halimi

---
