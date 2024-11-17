#include "crow.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>

using namespace std::chrono_literals;
using websocketpp::connection_hdl;
using websocketpp::frame::opcode::text;

typedef websocketpp::server<websocketpp::config::asio> server;
typedef websocketpp::client<websocketpp::config::asio> client;
typedef websocketpp::config::asio::message_type::ptr message_ptr;

// Globale Steuerungsvariablen
std::atomic<bool> stop_thread(false);
std::atomic<bool> active(true);

// --- Daten an lokale Clients senden ---
void send_sinus_data_to_clients(server &s, std::vector<connection_hdl> &connections, double frequency, double value_min, double value_max) {
    double time = 0.0;
    std::cout << "Sinuskurven-Thread gestartet!" << std::endl;

    while (active && !stop_thread) {
        std::this_thread::sleep_for(100ms);

        // Berechnung des Sinuswerts
        double amplitude = (value_max - value_min) / 2.0;
        double offset = (value_max + value_min) / 2.0;
        double value = amplitude * std::sin(frequency * time) + offset;
        time += 0.01;

        // Unix-Timestamp im Nanosekunden-Format erstellen
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

        // JSON-Nachricht erstellen
        std::string message = "{\"timestamp\": " + std::to_string(timestamp) + ", \"value\": " + std::to_string(value) + "}";

        // Sende Nachricht an alle verbundenen WebSocket-Clients
        for (auto const &conn : connections) {
            try {
                s.send(conn, message, text);
            } catch (const std::exception &e) {
                std::cerr << "Fehler beim Senden an Client: " << e.what() << std::endl;
            }
        }
    }

    std::cout << "Sinuskurven-Thread beendet." << std::endl;
}

// --- Daten an den Aggregator senden ---
void send_data_to_aggregator(client &agg_client, websocketpp::connection_hdl agg_hdl, double frequency, double value_min, double value_max) {
    double time = 0.0;
    std::cout << "Aggregator-Daten-Thread gestartet!" << std::endl;

    while (active && !stop_thread) {
        std::this_thread::sleep_for(10ms);

        // Berechnung des Sinuswerts
        double amplitude = (value_max - value_min) / 2.0;
        double offset = (value_max + value_min) / 2.0;
        double value = amplitude * std::sin(frequency * time) + offset;
        time += 0.01;

        // Unix-Timestamp im Nanosekunden-Format erstellen
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

        // JSON-Nachricht erstellen
        std::string message = "{\"timestamp\": " + std::to_string(timestamp) + ", \"value\": " + std::to_string(value) + "}";

        // Überprüfen, ob die Verbindung zum Aggregator offen ist
        if (agg_client.get_con_from_hdl(agg_hdl)->get_state() == websocketpp::session::state::open) {
            try {
                agg_client.send(agg_hdl, message, text);
                std::cout << "Gesendete Nachricht an Aggregator: " << message << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Fehler beim Senden an Aggregator: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Verbindung zum Aggregator nicht im 'offenen' Zustand!" << std::endl;
        }
    }

    std::cout << "Aggregator-Daten-Thread beendet." << std::endl;
}

// --- WebSocket-Server: Ereignisse ---
void on_open(server &s, connection_hdl hdl, std::vector<connection_hdl> &connections, std::mutex &conn_mutex) {
    std::lock_guard<std::mutex> lock(conn_mutex);
    connections.push_back(hdl);
    std::cout << "Neuer Client verbunden!" << std::endl;
}

void on_close(server &s, connection_hdl hdl, std::vector<connection_hdl> &connections, std::mutex &conn_mutex) {
    std::lock_guard<std::mutex> lock(conn_mutex);
    connections.erase(std::remove_if(connections.begin(), connections.end(),
                                     [hdl](const connection_hdl &conn) { return conn.lock() == hdl.lock(); }),
                      connections.end());
    std::cout << "Client getrennt!" << std::endl;
}

// --- Hauptfunktion ---
int main() {
    // WebSocket-Server initialisieren
    server s;
    client agg_client;
    std::vector<connection_hdl> connections;
    std::mutex conn_mutex;

    s.init_asio();
    agg_client.init_asio();

    // Event-Handler setzen
    s.set_open_handler([&s, &connections, &conn_mutex](connection_hdl hdl) { on_open(s, hdl, connections, conn_mutex); });
    s.set_close_handler([&s, &connections, &conn_mutex](connection_hdl hdl) { on_close(s, hdl, connections, conn_mutex); });

    // WebSocket-Server starten
    s.listen(8766);
    s.start_accept();

    // Konfiguration der Sinuskurve
    double frequency = 1.0;
    double value_min = 48.0;
    double value_max = 52.0;

    // Verbindung zum Aggregator herstellen
    websocketpp::lib::error_code ec;
    client::connection_ptr con = agg_client.get_connection("ws://localhost:8080/in", ec);
    if (ec) {
        std::cerr << "Fehler beim Erstellen der Verbindung: " << ec.message() << std::endl;
        return 1;
    }

    // Handlers für den Aggregator setzen
    agg_client.set_open_handler([&](connection_hdl hdl) {
        std::cout << "Verbindung zum Aggregator geöffnet!" << std::endl;
    });
    agg_client.set_fail_handler([&](connection_hdl hdl) {
        std::cerr << "Fehler bei der Verbindung zum Aggregator!" << std::endl;
    });
    agg_client.set_close_handler([&](connection_hdl hdl) {
        std::cerr << "Verbindung zum Aggregator geschlossen!" << std::endl;
    });

    // Verbindung starten
    agg_client.connect(con);
    websocketpp::connection_hdl agg_hdl = con->get_handle();

    // Aggregator-Client in eigenem Thread starten
    std::thread client_thread([&agg_client]() {
        agg_client.run();
    });

    // Threads für Sinuskurve und Aggregator
    std::thread sinus_thread(send_sinus_data_to_clients, std::ref(s), std::ref(connections), frequency, value_min, value_max);
    std::thread aggregator_thread(send_data_to_aggregator, std::ref(agg_client), agg_hdl, frequency, value_min, value_max);

    // WebSocket-Server starten
    std::cout << "WebSocket-Server läuft auf ws://localhost:8766" << std::endl;
    s.run();

    // Beenden der Threads
    stop_thread.store(true);
    if (sinus_thread.joinable()) sinus_thread.join();
    if (aggregator_thread.joinable()) aggregator_thread.join();
    if (client_thread.joinable()) client_thread.join();

    return 0;
}

