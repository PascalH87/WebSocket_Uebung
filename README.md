# WebSocket_Uebung
Ein WebSocket-Datenaggregator mit REST-API und Visualisierung

WebSocket Client für Aggregator und Datenserver
Dieses Projekt implementiert eine grafische Benutzeroberfläche (GUI) für die Kommunikation mit einem WebSocket-Server und zwei REST-APIs: einen Aggregator und einen Datenserver. Der Client ermöglicht es, die WebSocket-Verbindung zum Aggregator zu starten, Daten von diesem zu empfangen und in einem Plot anzuzeigen. Außerdem können Benutzer den Aggregator und den Datenserver über die REST-API steuern.

Funktionen
Starten des Aggregators über die REST-API.
Starten des Datenservers über die REST-API mit der Möglichkeit, ID und Info zu übergeben.
Verbindung zum Aggregator über WebSocket und Empfangen von Daten.
Visualisierung der empfangenen Daten in Echtzeit mit Matplotlib.
Anforderungen
Python 3.6 oder höher
Tkinter (für die GUI)
Matplotlib (für die Visualisierung der Daten)
WebSocket (für die WebSocket-Verbindung)
Requests (für die REST-API-Kommunikation)
subprocess (um externe Prozesse zu starten)
Installation
1. Python-Umgebung einrichten
Falls noch nicht geschehen, installieren Sie Python 3.6 oder höher.

2. Abhängigkeiten installieren
Erstellen Sie eine virtuelle Umgebung (optional, aber empfohlen):

python -m venv venv

Aktivieren Sie die virtuelle Umgebung:

Windows:

venv\Scripts\activate

Linux/macOS:

source venv/bin/activate

Installieren Sie dann die erforderlichen Python-Pakete:

pip install -r requirements.txt

Erstellen Sie eine requirements.txt mit den folgenden Abhängigkeiten:

requests
websocket-client
matplotlib

3. REST-API Server und Aggregator vorbereiten
Stellen Sie sicher, dass der REST-API-Server und der Aggregator auf localhost verfügbar sind und auf den richtigen Ports laufen:
Aggregator: http://localhost:18080/start_aggregator
Datenserver: http://localhost:18080/start_data_server

4. REST-API Server ausführen
Sie müssen den REST-API Server in einer separaten Konsole starten, bevor Sie den Client verwenden. Dieser Server wird über die subprocess-Funktion des Clients gestartet, aber Sie können ihn auch manuell ausführen, wenn gewünscht.

Falls der Server in einem Ordner ~/WebSocket_Übung/REST_Api/build/ unter dem Namen CrowRestApi kompiliert wurde, können Sie diesen Server folgendermaßen starten:

cd ~/WebSocket_Übung/REST_Api/build/
./CrowRestApi

5. WebSocket-Server
Der WebSocket-Server muss ebenfalls laufen, bevor Sie den WebSocket-Client verwenden können. Stellen Sie sicher, dass er auf ws://localhost:8080/out erreichbar ist.

Verwendung
Starten des Aggregators:

Klicken Sie auf den Button "Start Aggregator", um den Aggregator über die REST-API zu starten.
Starten des Datenservers:^

Klicken Sie auf den Button "Start Data Server", um den Datenserver zu starten. Der Server erwartet JSON-Daten (ID und Info).
WebSocket-Verbindung:

Klicken Sie auf den Button "Connect to Aggregator", um eine WebSocket-Verbindung zum Aggregator herzustellen.
Wenn die Verbindung erfolgreich hergestellt wird, empfangen Sie Echtzeitdaten vom Aggregator.
Datenvisualisierung:

Die empfangenen Daten werden automatisch in einem Diagramm angezeigt.
