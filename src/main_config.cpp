// #include "../include/WebServer.hpp"


// int     main(int argc, char **argv)
// {
//         std::string configPath = "config.conf"; 
//         if (argc == 2)
//                 configPath = argv[1];
//         else if (argc > 2)
//         {
//                 std::cerr << "Usage: ./webserv [config_file]" << std::endl;
//                 return 1;
//         }
//         try {
//                 WebServer myserver;
//                 myserver.setupServer(configPath);
//                 std::srand(std::time(NULL));
//                 myserver.runServer();
//         }
//         catch(const std::exception& e)
//         {
//                 std::cerr << e.what() << '\n';
//         }
//         return 0;
// }
#include "../include/Request.hpp"
#include <fstream>

static void run_test(const std::string &name, std::string buffer)
{
	std::cout << "\n====== TEST: " << name << " ======\n";
	Request req;
	parse_request(buffer, req);
	req.print();

	// Si un fichier body a été créé, afficher son contenu
	if (!req.getBody().empty() && req.isComplete())
	{
		std::ifstream f(req.getBody().c_str());
		if (f.is_open())
		{
			std::string content((std::istreambuf_iterator<char>(f)),
			                     std::istreambuf_iterator<char>());
			std::cout << "Body file content: |" << content << "|\n";
			f.close();
			std::remove(req.getBody().c_str()); // cleanup
		}
	}
}

int main(void)
{
	// ── 1. GET simple ──
	run_test("GET simple",
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Connection: close\r\n"
		"\r\n");

	// ── 2. GET avec query string ──
	run_test("GET avec query",
		"GET /search?q=hello&lang=fr HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

	// ── 3. POST body exact ──
	run_test("POST body exact",
		"POST /form HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"Content-Length: 7\r\n"
		"\r\n"
		"a=1&b=2");

	// ── 4. DELETE ──
	run_test("DELETE",
		"DELETE /file.txt HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

	// ── 5. Methode invalide ──
	run_test("Methode invalide",
		"PATCH /resource HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n");

	// ── 6. Version invalide ──
	run_test("Version invalide",
		"GET /index HTTP/2.0\r\n"
		"Host: localhost\r\n"
		"\r\n");

	// ── 7. POST sans Content-Type ──
	run_test("POST sans Content-Type",
		"POST /form HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello");

	// ── 8. POST sans Content-Length ──
	run_test("POST sans Content-Length",
		"POST /form HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n");

	// ── 9. Multi-chunk ──
	std::cout << "\n====== TEST: multi-chunk ======\n";
	{
		Request req;
		std::string accumulated = "";

		std::string chunk1 =
			"POST /upload HTTP/1.1\r\n"
			"Host: localhost\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: 5\r\n";
		std::string chunk2 = "\r\nhello";

		std::cout << "-- Chunk 1 --\n";
		accumulated += chunk1;
		if (accumulated.find("\r\n\r\n") != std::string::npos)
			parse_request(accumulated, req);
		std::cout << "Status apres chunk1: " << req.getStatus() << "\n";

		std::cout << "-- Chunk 2 --\n";
		accumulated += chunk2;
		parse_request(accumulated, req);
		req.print();

		// Lire le fichier body
		if (!req.getBody().empty() && req.isComplete())
		{
			std::ifstream f(req.getBody().c_str());
			if (f.is_open())
			{
				std::string content((std::istreambuf_iterator<char>(f)),
				                     std::istreambuf_iterator<char>());
				std::cout << "Body file content: |" << content << "|\n";
				f.close();
				std::remove(req.getBody().c_str());
			}
		}
	}

	return 0;
}