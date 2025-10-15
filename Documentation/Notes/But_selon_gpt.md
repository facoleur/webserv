🧠 1. Le but du projet

Le but est d’implémenter ton propre serveur HTTP, en C++98, capable de :
	•	Accepter des connexions depuis un navigateur web (ou curl, telnet, etc.),
	•	Lire et interpréter des requêtes HTTP (GET, POST, DELETE),
	•	Y répondre correctement avec les bons codes de statut (200 OK, 404 Not Found, etc.),
	•	Gérer plusieurs connexions simultanées sans bloquer (grâce à poll(), select(), epoll(), ou kqueue()),
	•	Et rester stable (ne jamais crasher, même en cas d’erreur ou de surcharge).

👉 Autrement dit :
Tu recrées une version simplifiée de NGINX ou Apache, mais sans leurs complexités internes.

⸻

⚙️ 2. Les fonctionnalités principales à implémenter

Voici ce que ton serveur doit obligatoirement faire :

A. Fonctionnement général
	•	Lire un fichier de configuration (config.conf) pour savoir :
	•	Sur quels ports et interfaces écouter,
	•	Quelles routes sont disponibles et comment les traiter,
	•	Les limites de taille pour les requêtes,
	•	Les pages d’erreur personnalisées,
	•	Où stocker les fichiers uploadés,
	•	Les redirections éventuelles (301, 302…),
	•	Si le listing de répertoire est autorisé,
	•	Et éventuellement un fichier par défaut à servir (index.html).

B. Gestion HTTP

Ton serveur doit :
	•	Recevoir des requêtes HTTP 1.0 / 1.1
	•	Gérer au minimum 3 méthodes :
	•	GET → renvoyer une ressource (fichier, page)
	•	POST → accepter des données (formulaire, upload)
	•	DELETE → supprimer un fichier côté serveur
	•	Répondre avec les bons headers (Content-Length, Content-Type, etc.)
	•	Fournir des pages d’erreur par défaut si la ressource n’existe pas.

C. Comportement non-bloquant
	•	Tu dois utiliser un seul poll() (ou équivalent) pour gérer toutes les connexions (écoute + lecture + écriture).
	•	Aucune lecture/écriture directe sans passer par ce mécanisme.
	•	Chaque client doit être traité sans bloquer les autres.

D. CGI (Common Gateway Interface)
	•	Tu dois permettre l’exécution d’un script CGI (par exemple un script .php ou .py).
	•	Le serveur doit :
	•	Fournir au CGI les bonnes variables d’environnement (méthode, chemin, etc.),
	•	Lire la sortie du CGI et la renvoyer au client,
	•	Gérer les cas chunked (corps de requêtes fractionnés),
	•	Et exécuter le CGI dans le bon répertoire.

⸻

🧩 3. Structure logique du projet

Pour comprendre comment construire le projet, voici la vision modulaire :

🧱 1. Parsing du fichier de configuration
	•	Lire le fichier .conf (inspiré d’un nginx.conf)
	•	En extraire les directives (ports, routes, erreurs, méthodes, CGI, etc.)
	•	Créer des objets Serveur et Location (pour les routes)

🔌 2. Serveur / Sockets
	•	Créer les sockets d’écoute sur les ports définis
	•	Les rendre non-bloquantes
	•	Ajouter tous les file descriptors au poll() principal

🔄 3. Boucle d’événements (poll)
	•	Écouter en continu les événements :
	•	Nouvelle connexion → accept()
	•	Lecture sur une socket → recv()
	•	Écriture disponible → send()
	•	Chaque client doit être représenté par une structure (Client, Connection, etc.) pour suivre son état (lecture/écriture, requête complète, etc.)

📬 4. Parsing des requêtes HTTP
	•	Lire la requête du client,
	•	La découper en :
	•	Ligne de requête (méthode + chemin + version)
	•	En-têtes
	•	Corps
	•	Vérifier les erreurs (méthode non autorisée, taille trop grande, fichier manquant…)

📤 5. Construction des réponses
	•	Générer les bonnes réponses HTTP
	•	Headers + body
	•	Statuts (200, 404, 405, 413, etc.)
	•	Si c’est un CGI, attendre sa sortie
	•	Envoyer la réponse au client

🧠 6. Gestion des erreurs & résilience
	•	Jamais de crash
	•	Timeout géré
	•	Bonne fermeture des sockets
	•	Stress test recommandé

⸻

🎁 4. Bonus possibles

Si vous terminez le mandatory :
	•	Gestion de cookies / sessions
	•	Plusieurs types de CGI
	•	Support HTTP 1.1 complet (Keep-Alive, chunked encoding, etc.)
	•	Virtual hosts (plusieurs sites sur un même serveur)

⸻

🚀 5. Enjeux pédagogiques

Ce projet est un énorme pas vers la programmation système :
	•	Tu comprends comment un serveur web fonctionne réellement (sockets, poll, HTTP),
	•	Tu apprends à écrire un programme asynchrone et robuste,
	•	Et tu touches à la séparation claire des responsabilités (config, parsing, logique réseau, HTTP, CGI).

C’est aussi un projet de travail d’équipe :
	•	Chaque membre peut se spécialiser :
	•	🧾 Parsing du fichier config
	•	🌐 Gestion réseau et poll
	•	📦 Parsing / construction HTTP
	•	⚙️ CGI / tests
