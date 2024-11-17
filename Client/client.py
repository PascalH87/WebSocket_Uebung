import tkinter as tk
from tkinter import messagebox
import websocket
import threading
import json
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from datetime import datetime
import requests
import subprocess
import time
import os


class RingBuffer:
    def __init__(self, size=1000):
        self.size = size
        self.values = [None] * size
        self.timestamps = [None] * size
        self.index = 0

    def push_front(self, value, timestamp):
        self.values[self.index] = value
        self.timestamps[self.index] = timestamp
        self.index = (self.index + 1) % self.size

    def get_data(self):
        values = self.values[self.index:] + self.values[:self.index]
        timestamps = self.timestamps[self.index:] + self.timestamps[:self.index]
        return values, timestamps


class WebSocketClientApp:
    def __init__(self, master):
        self.master = master
        self.master.title("WebSocket Client")
        self.master.geometry("800x600")

        # REST-API Buttons
        rest_frame = tk.Frame(master)
        rest_frame.pack(pady=10)

        self.start_aggregator_button = tk.Button(rest_frame, text="Start Aggregator", command=self.start_aggregator, width=20, height=2)
        self.start_aggregator_button.grid(row=0, column=0, padx=10)

        self.start_data_server_button = tk.Button(rest_frame, text="Start Data Server", command=self.start_data_server, width=20, height=2)
        self.start_data_server_button.grid(row=0, column=1, padx=10)

        # WebSocket Connection
        self.ws_uri = "ws://localhost:8080/out"  # URI für den Aggregator
        self.websocket_connected = False
        self.ring_buffers = {}  # UUID -> RingBuffer

        # WebSocket Buttons
        ws_frame = tk.Frame(master)
        ws_frame.pack(pady=10)

        self.connect_ws_button = tk.Button(ws_frame, text="Connect to Aggregator", command=self.connect_to_aggregator, width=20, height=2)
        self.connect_ws_button.grid(row=0, column=0, padx=10)

        # Plot Area
        self.figure, self.ax = plt.subplots(figsize=(8, 5))
        self.canvas = FigureCanvasTkAgg(self.figure, master)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        # WebSocket Thread
        self.websocket_thread = None
        self.running = False

        # REST-API Server starten
        self.start_rest_api_server()

        # Bind the window close event to handle the cleanup
        self.master.protocol("WM_DELETE_WINDOW", self.on_closing)

    def start_rest_api_server(self):
        """ Startet den REST-API Server im Hintergrund """
        try:
            # Pfad zur REST-API Server ausführbaren Datei anpassen
            rest_api_server_path = os.path.expanduser("~/WebSocket_Übung/REST_Api/build/CrowRestApi")
            self.rest_api_process = subprocess.Popen([rest_api_server_path])
            time.sleep(2)  # Gebe dem Server Zeit, um zu starten
        except Exception as e:
            messagebox.showerror("Error", f"Failed to start REST API server: {e}")

    def start_aggregator(self):
        """Startet den Aggregator über REST-API"""
        try:
            response = requests.post("http://localhost:18080/start_aggregator")
            if response.status_code == 200:
                messagebox.showinfo("Success", "Aggregator started successfully.")
            else:
                messagebox.showerror("Error", f"Failed to start aggregator: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"An error occurred: {e}")

    def start_data_server(self):
        """Startet den Datenserver über REST-API"""
        try:
            data = {
                "id": 1,
                "info": "Data server 1"
            }
            response = requests.post("http://localhost:18080/start_data_server", json=data)
            if response.status_code == 200:
                messagebox.showinfo("Success", "Data server started successfully.")
            else:
                messagebox.showerror("Error", f"Failed to start data server: {response.text}")
        except Exception as e:
            messagebox.showerror("Error", f"An error occurred: {e}")

    def connect_to_aggregator(self):
        """Verbindet sich mit dem Aggregator über WebSocket"""
        if self.websocket_connected:
            messagebox.showinfo("Info", "Already connected to the aggregator.")
            return

        self.websocket_thread = threading.Thread(target=self.run_websocket)
        self.websocket_thread.start()

    def run_websocket(self):
        """Führt die WebSocket-Verbindung aus"""
        self.running = True
        websocket.enableTrace(False)
        ws = websocket.WebSocketApp(self.ws_uri,
                                    on_message=self.on_message,
                                    on_error=self.on_error,
                                    on_close=self.on_close)
        ws.on_open = self.on_open
        self.ws = ws
        ws.run_forever()

    def on_message(self, ws, message):
        """Verarbeitet eingehende WebSocket-Nachrichten"""
        try:
            data = json.loads(message)
            uuid = data['uuid']
            value = data['value']
            timestamp = data['timestamp']

            if uuid not in self.ring_buffers:
                self.ring_buffers[uuid] = RingBuffer()

            self.ring_buffers[uuid].push_front(value, timestamp)
            self.update_plot()
        except (json.JSONDecodeError, KeyError) as e:
            print(f"Error processing message: {e}")

    def on_error(self, ws, error):
        """Fehlerbehandlung bei WebSocket"""
        print("WebSocket Error:", error)

    def on_close(self, ws):
        """Beendet die WebSocket-Verbindung"""
        print("WebSocket connection closed.")
        self.websocket_connected = False
        self.running = False

    def on_open(self, ws):
        """Bestätigt den Verbindungsaufbau mit dem WebSocket-Server"""
        print("WebSocket connection established.")
        self.websocket_connected = True

    def update_plot(self):
        """Aktualisiert das Plot mit neuen Daten"""
        self.ax.clear()

        for uuid, buffer in self.ring_buffers.items():
            values, timestamps = buffer.get_data()
            valid_indices = [i for i, v in enumerate(values) if v is not None]
            valid_values = [values[i] for i in valid_indices]
            valid_timestamps = [timestamps[i] for i in valid_indices]

            self.ax.plot(valid_timestamps, valid_values, label=f"UUID: {uuid}")

        self.ax.set_title("Received Data")
        self.ax.set_xlabel("Timestamp")
        self.ax.set_ylabel("Value")
        self.ax.legend()
        self.canvas.draw()

    def on_closing(self):
        """Bereinigt Ressourcen beim Schließen der GUI"""
        # Wenn der WebSocket läuft, schließen wir ihn
        if self.websocket_thread and self.websocket_thread.is_alive():
            self.ws.close()
            self.websocket_thread.join()

        # Beende den REST-API Server, wenn der Client geschlossen wird
        if hasattr(self, 'rest_api_process'):
            self.rest_api_process.terminate()

        self.master.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = WebSocketClientApp(root)
    root.mainloop()

