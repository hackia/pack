#include "../include/Sender.hpp"

#include "../include/Pack.hpp"
using namespace k;
using namespace std;

namespace fs = std::filesystem;

Sender::Sender(const vector<byte> &files) : to_send(files) {
}

int Sender::send() {
    Pack::ok("start files transfer");
    for (const auto &x: to_send) {
    }
    return 0;
}


bool write_content_to_file(const fs::path &file_path, const std::vector<std::byte> &content) {
    try {
        const auto parent_dir = file_path.parent_path();
        if (!parent_dir.empty() && fs::exists(parent_dir)) {
            fs::create_directories(parent_dir);
        }
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Erreur (filesystem) : Impossible de creer les dossiers : " << e.what() << std::endl;
        return false;
    }

    std::ofstream outfile(file_path, std::ios::binary | std::ios::out);

    if (!outfile.is_open()) {
        std::cerr << "Erreur : Impossible d'ouvrir le fichier en ecriture : " << file_path << std::endl;
        return false;
    }

    // Écrire TOUT le vecteur d'un seul coup
    // On doit re-caster les std::byte en const char* pour l'API de std::ofstream
    try {
        outfile.write(reinterpret_cast<const char *>(content.data()), content.size());
    } catch (const std::exception &e) {
        std::cerr << "Erreur pendant l'ecriture dans le fichier : " << e.what() << std::endl;
        outfile.close();
        return false;
    }

    outfile.close();

    if (outfile.fail()) {
        std::cerr << "Erreur : L'ecriture dans le fichier a echoue (failbit) : " << file_path << std::endl;
        return false;
    }

    std::cout << "Succes : Fichier enregistre : " << file_path << std::endl;
    return true;
}


// --- NOUVELLE FONCTION "WRAPPER" ASYNCHRONE ---

/**
 * \brief Écrit un vecteur de bytes dans un fichier de manière ASYNCHRONE.
 *
 * Cette fonction est une coroutine (C++20). Elle ne bloque pas le
 * thread appelant (ex: le thread réseau).
 * Elle "poste" le travail bloquant (write_content_to_file) sur un
 * pool de threads séparé.
 *
 * \param pool Le pool de threads sur lequel exécuter l'écriture disque.
 * \param file_path Le chemin du fichier à écrire.
 * \param content Le contenu binaire.
 * \return asio::awaitable<bool> - 'true' en cas de succès, 'false' sinon.
 */
asio::awaitable<bool> async_write_to_file(
    asio::thread_pool &pool,
    const fs::path &file_path,
    // Note : on prend par copie (ou std::move) pour que les données
    // vivent assez longtemps pour le thread worker.
    const std::vector<std::byte> &content) {
    std::cout << "async_write: Demarrage (thread principal...)" << std::endl;

    // --- C'est la "magie" ---
    // co_await asio::dispatch :
    // 1. Prend le lambda (la fonction)
    // 2. L'envoie au pool de threads "pool"
    // 3. Suspend cette coroutine (sans bloquer le thread)
    // 4. Reprend ici quand le lambda est fini
    // 5. Retourne ce que le lambda a retourné (ici, un bool)
    bool result = co_await asio::dispatch(
        pool.get_executor(), // Exécute le lambda sur ce pool
        [=]() {
            // Ce code s'exécute sur un thread du pool
            std::cout << "async_write: Ecriture sur le thread pool..." << std::endl;
            return write_content_to_file(file_path, content);
        },
        asio::use_awaitable // Permet à co_await d'attendre ce lambda
    );

    std::cout << "async_write: Ecriture terminee (retour sur thread principal...)" << std::endl;

    // co_return transmet le résultat à celui qui nous a "await"
    co_return result;
}


// --- NOUVEL EXEMPLE D'UTILISATION (main) ---
int main() {
    std::cout << "--- Test ASYNCHRONE d'ecriture de fichier ---" << std::endl;

    // 1. Contexte I/O principal (pour le "main" et le réseau)
    asio::io_context io_ctx;

    // 2. Pool de threads pour le travail bloquant (le disque)
    //    On ne veut pas bloquer l'io_ctx principal !
    asio::thread_pool disk_pool(4); // 4 threads pour le disque

    // 3. Créer un faux contenu de fichier
    std::vector<std::byte> mon_contenu = {
        std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF},
        std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD}
    };
    fs::path chemin_destination = "./mon_objet_async.dat";


    // 4. Lancer la coroutine "principale"
    // co_spawn lance une coroutine sur un contexte (io_ctx)
    asio::co_spawn(io_ctx,
                   // Le lambda ici EST notre coroutine
                   [&]() -> asio::awaitable<void> {
                       std::cout << "[main coroutine] Lancement de async_write_to_file..." << std::endl;

                       // 5. On "await" (attend sans bloquer) notre fonction
                       bool succes = co_await async_write_to_file(
                           disk_pool,
                           chemin_destination,
                           mon_contenu
                       );

                       if (succes) {
                           std::cout << "[main coroutine] Test reussi." << std::endl;
                       } else {
                           std::cout << "[main coroutine] Test echoue." << std::endl;
                       }

                       // 6. On arrête la boucle d'événements
                       io_ctx.stop();
                   },
                   // Gérer les exceptions de la coroutine
                   [](std::exception_ptr e) {
                       if (e) {
                           std::rethrow_exception(e);
                       }
                   }
    );

    // 7. Mettre en route la boucle d'événements Asio
    // Elle va tourner jusqu'à ce que io_ctx.stop() soit appelée
    std::cout << "[main thread] Lancement de io_ctx.run()..." << std::endl;
    io_ctx.run();

    std::cout << "[main thread] io_ctx.run() est termine." << std::endl;
    disk_pool.join(); // Attendre que le pool de threads se termine

    return 0;
}



