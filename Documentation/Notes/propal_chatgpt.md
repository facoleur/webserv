# Webserv — Architecture, plan et répartition d’équipe

> Objectif: fournir une base commune claire (théorie + conception) pour lancer le projet sans coder, faciliter la répartition des rôles et cadrer les décisions techniques majeures.

---

## 1) Vision & exigences clés (rappel synthétique)

* Serveur HTTP minimaliste en **C++98**, **non-bloquant**, un **seul poll()/équivalent** pour lecture/écriture + écoute.
* Méthodes: **GET**, **POST**, **DELETE**.
* **CGI**: au moins un type (ex: php-cgi ou python), variables d’environnement correctes, gestion du body (chunked → unchunk côté serveur), exécution dans le bon répertoire.
* **Fichier de configuration** inspiré de NGINX: ports/interfaces, routes (locations), méthodes autorisées, redirections, root, autoindex, index par défaut, upload path, error pages, client_max_body_size, multi-ports.
* **Compatibilité navigateur**, **codes HTTP justes**, **pages d’erreur par défaut**.

---

## 2) Architecture d’ensemble (vue logique)

```
+-------------------+        +-------------------+        +-----------------------+
|   Config Parser   |  --->  |  Server Manager   |  --->  |    EventLoop (poll)   |
+-------------------+        +-------------------+        +-----------+-----------+
           |                           |                               |
           v                           v                               v
   +---------------+          +----------------+               +---------------+
   | Config Model  |          | Listener Sockets|              |  ClientState  |
   +---------------+          +----------------+               +-------+-------+
                                                                             |
                                                                             v
                                                                  +-------------------+
                                                                  | HTTP Pipeline     |
                                                                  |(Parse → Route →   |
                                                                  | CGI? → Respond)   |
                                                                  +-------------------+
```

**Flux**: Config → sockets d’écoute → boucle `poll()` → états clients → parse requête → route/location → (statique | upload | delete | CGI) → build réponse → envoi non-bloquant.

---

## 3) Modules & classes (proposition)

### 3.1 Core

* **`ConfigParser`**: lit le .conf, remplit le modèle.
* **`Config` / `ServerBlock` / `LocationBlock`**: structures immuables une fois chargées.
* **`ServerManager`**: crée les sockets d’écoute (par port/interface), applique `O_NONBLOCK`.
* **`EventLoop`** (select/poll/epoll/kqueue): unique point d’attente I/O, distribue les événements.
* **`Client`**: état par connexion (buffers in/out, timestamps, state machine HTTP, file descriptors temporaires pour upload/CGI, etc.).

### 3.2 HTTP

* **`HttpRequestParser`**: stateful (ligne de requête → headers → body). Gère `Content-Length`, `Transfer-Encoding: chunked` (unchunk). Limites (taille max body).
* **`HttpRequest` / `HttpResponse`**: POJOs.
* **`Router`**: trouve `LocationBlock` selon l’URL, applique méthodes autorisées, redirections, root, index, autoindex, upload.
* **`StaticHandler`**: sert fichiers, répertoires (autoindex), 404/403, MIME type.
* **`UploadHandler`**: écrit vers dossier uploads (en stream), valide taille.
* **`DeleteHandler`**: supprime resource si autorisé.
* **`CgiHandler`**: fork/execve du binaire CGI, set env (REQUEST_METHOD, SCRIPT_FILENAME, QUERY_STRING, CONTENT_LENGTH, CONTENT_TYPE, SERVER_NAME, etc.), pipe I/O non-bloquants, collecte sortie, parse headers CGI (Status/Content-Type), fallback EOF.
* **`ErrorPage`**: rendu par défaut + surcharge via config.

### 3.3 Utilitaires

* **`FdSet`/`Poller`** (abstraction sur select/poll/epoll/kqueue).
* **`Buffer`** (ring/simple), **`Timer`** (timeouts par client), **`MimeTypes`** (ext → content-type), **`Path`** util.
* **`Logger`** minimal (niveau INFO/ERROR, timestamps).

---

## 4) États & transitions (state machine par client)

États proposés:

* `RECV_REQUEST_LINE`
* `RECV_HEADERS`
* `RECV_BODY` (content-length | chunked)
* `ROUTING`
* `PROCESS_STATIC` | `PROCESS_UPLOAD` | `PROCESS_DELETE` | `PROCESS_CGI`
* `BUILD_RESPONSE_HEADERS`
* `SEND_RESPONSE`
* `CLOSE_OR_KEEPALIVE`

Transitions déclenchées par:

* Readable/Writeable sur socket
* Buffer complet / request complete
* Fin de CGI / data disponible
* Timeout

---

## 5) Boucle d’événements (pseudo-code)

```cpp
while (running) {
  poller.wait();
  for (fd in poller.readables()) {
    if (fd is listener) accept_nonblock();
    else client.onReadable();
  }
  for (fd in poller.writables()) {
    client.onWritable();
  }
  for (client in clients) {
    client.advanceState(); // parse, route, schedule handlers, timeouts
  }
}
```

**Règles d’or**:

* Jamais `read`/`write` sans readiness.
* Un seul mécanisme d’attente global.
* Les fichiers réguliers peuvent être lus/écrits directement (pas besoin de poller).

---

## 6) Parsing HTTP (détails utiles)

* **Taille max header line** (ex. 8KB) & **header total** (ex. 16–32KB) pour éviter abus.
* Normaliser CRLF (`\r\n`). Stop headers sur ligne vide.
* `Host` requis en HTTP/1.1; supporter HTTP/1.0 (selon choix d’équipe).
* **Body**:

  * `Content-Length`: lire N octets.
  * `Transfer-Encoding: chunked`: parser taille hex → chunks → agrégation → EOF.
* Conserver `Connection: keep-alive/close` et décider fermeture après réponse.

---

## 7) Routing & handlers

* **Match location** (préfixe le plus long).
* Vérifier **méthode autorisée** (405 si refusée; renvoyer `Allow`).
* **Redirection** (3xx) si configurée.
* **Static**: `root + path` → si dossier: `index` ou **autoindex** (listing HTML simple).
* **Upload**: si autorisé et POST, écrire vers `upload_store`.
* **Delete**: vérifs d’accès, stat(), unlink().
* **CGI**: si extension match (ex: `.php`), déléguer via `CgiHandler`.

---

## 8) CGI (points d’attention)

* **Env vars** minimales: `GATEWAY_INTERFACE=CGI/1.1`, `REQUEST_METHOD`, `SCRIPT_FILENAME`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`, `SERVER_PROTOCOL`, `SERVER_NAME`, `SERVER_PORT`, `PATH_INFO` (si pertinent), `REDIRECT_STATUS=200` (php-cgi), etc.
* **Entrée**: body de la requête (fournir via stdin, non-bloquant ou buffer fichier).
* **Sortie**: lire stdout du CGI, parser entêtes (ex: `Status: 200 OK`, `Content-Type: text/html`). Si pas de `Content-Length`, envoyer jusqu’à EOF.
* Exécuter dans le **bon cwd** (pour chemins relatifs).

---

## 9) Fichier de configuration (mini-grammaire + exemple)

### 9.1 Mini-grammaire (style NGINX simplifié)

```
server {
  listen 0.0.0.0:8080;
  server_name mysite;             # optionnel
  client_max_body_size 10M;       
  error_page 404 /errors/404.html;

  location / {
    root /var/www/site1;          
    index index.html;             
    autoindex off;                
    allowed_methods GET POST;     
  }

  location /upload {
    root /var/www/site1;         
    allowed_methods POST;        
    upload_store /var/www/uploads;
  }

  location /delete {
    root /var/www/site1;         
    allowed_methods DELETE;      
  }

  location /app {
    root /var/www/site1;
    cgi_extension .php;          
    cgi_path /usr/bin/php-cgi;   
  }

  location /old {
    return 301 /new;             # redirection
  }
}
```

### 9.2 Décisions à consigner

* Taille max requête (global + par location).
* Politique keep-alive (timeout, max-requests).
* Mapping MIME (fichier `mime.types` ou map minimal interne).
* Politique d’erreurs par défaut vs. custom.

---

## 10) Layout du repo & build

```
./Makefile
./include/
  config/*.hpp
  core/*.hpp
  http/*.hpp
  cgi/*.hpp
  util/*.hpp
./src/
  config/*.cpp
  core/*.cpp
  http/*.cpp
  cgi/*.cpp
  util/*.cpp
./conf/
  default.conf
  mime.types (optionnel)
./www/
  site1/... (fichiers de test)
./errors/
  404.html 405.html 413.html 500.html ...
./tests/
  scripts python/go pour stress & e2e
```

---

## 11) Plan de tests (théorique)

**Manuels**

* `curl -v` GET fichier / dossier (index + autoindex on/off).
* POST form + upload (petit & gros; limites; `chunked`).
* DELETE sur fichier autorisé/interdit.
* Redirections 301/302.
* CGI simple (echo headers/body), php-cgi si choisi.
* Multi-ports (ex: 8080 & 8081) → contenus différents.
* Timeouts client, fermeture propre; keep-alive.

**Automatisés** (scripts)

* Concurrence: 100-500 connexions (ab, wrk, ou script maison).
* Réponses codes HTTP cohérents.
* Comparaison d’entêtes vs NGINX sur cas simples.

---

## 12) Répartition d’équipe (suggestion)

* **Mate A — Réseau & EventLoop**: sockets, non-blocking, poller, lifecycle clients, timeouts.
* **Mate B — HTTP Core**: parser requêtes, router, réponses, handlers statique/upload/delete, MIME, erreurs.
* **Mate C — Config & CGI**: parsing conf + modèle; CGI (env, exec, pipes), intégration.

> Tous: pages d’erreur, www de test, scripts de tests.

---

## 13) Jalons & “Definition of Done”

**M1 (Jour 1–2)**: Config minimal + sockets écoute + event loop écho.
**M2**: Parser requête (ligne+headers) + GET statique simple.
**M3**: Headers complets + MIME + erreurs + index/autoindex.
**M4**: Body POST (`Content-Length` puis `chunked`) + upload.
**M5**: DELETE + limites tailles + codes d’erreur.
**M6**: CGI (pipeline complet) + variables env + tests.
**M7**: Multi-ports + redirections + polish (keep-alive, timeouts, pages erreur).

**DoD** par feature: tests `curl` + test navigateur + script e2e + pas de blocage, pas de fuite FD, codes HTTP corrects.

---

## 14) Pièges & décisions à trancher tôt

* **Un seul poller** et zéro I/O réseau sans readiness.
* **Gestion `chunked`** (requête) et contenu CGI sans `Content-Length` (EOF).
* **Chemins & sécurité**: empêcher `..` (path traversal), root propre.
* **Taille buffers** (headers/body) & stratégie (fichier temporaire pour gros body).
* **Time-out** (lecture, écriture, CGI) pour éviter sockets zombies.
* **SIGCHLD** & nettoyage des processus CGI.
* **Keep-Alive**: politique simple (limiter à N réponses ou timeout court).

---

## 15) Annexes utiles

* **Map MIME minimale**: `.html -> text/html`, `.css -> text/css`, `.js -> application/javascript`, `.png -> image/png`, `.jpg -> image/jpeg`, `.gif -> image/gif`, `.svg -> image/svg+xml`, `.json -> application/json`, `.txt -> text/plain`.
* **Codes d’erreur à prévoir**: 200, 201, 204, 301, 302, 303, 304, 307, 308, 400, 403, 404, 405 (+ `Allow`), 413, 414, 431, 500, 501, 502, 503, 504.

---

### Checklist de réunion (1h)

1. Valider choix poller (poll/select/kqueue/epoll selon OS).
2. Décider le type de CGI (php-cgi vs python) et son chemin.
3. Geler la mini-grammaire du fichier de config.
4. Split des rôles (A/B/C) + jalons M1–M3 immédiats.
5. Définir tailles limites (headers/body) et politiques keep-alive/timeouts.
6. Décider stratégie upload (fichier tmp + move) et dossier d’uploads.
7. Fixer structure du repo & conventions (logs, erreurs, tests).
