Sieci Komputerowe 2 - laboratorium
Sławomir Staniszewski
nr albumu 135893
semestr V, niestacjonarne, kierunek Informatyka, grupa L2

==================================================
Temat zadania:
Gra logiczna lub zręcznościowa - warcaby

==================================================
Opis protokołu komunikacyjnego:
Program używa protokołu TCP, by przesyłać ruchy między klientami a serwerem.
Serwer czeka na połączenie dwóch graczy, wita ich oraz przesyła im stringa reprezentującego rozstawienie pionków - na początku gry oraz po każdym wykonanym ruchu.
Klienci wykonują swoje ruchy za pomocą myszy w aplikacji napisanej w pythonie.

==================================================
Opis implementacji:
Serwer (server.c):
 - Uruchomienie serwera umieszczono w funkcji main(), gdzie konfigurowane są parametry połączenia. Obsługa konkretnego połączenia znajduje się w funkcji void *connection_handler(void *socket_desc)
 - Funkcja connection_handler przydziela gracza do wolnej gry i obsługuje komunikację z graczem
 - Logika gry w warcaby przechowywana jest na serwerze. Do jej obsługi służą funkcje CreateTable, ShowBoard, MakeMove, isValidMove, CanCaptureFromPosition, Can Capture, PerformCapture, PromoteToKing, CheckWin

Klient (client.py):
 - Konfiguracja połączenia następuje w funkcji connect_to_server(self, ip_address, port)
 - W pętli while ma miejsce nasłuchiwanie na wiadomości od serwera (na przykład aktualny stan planszy)
 - Ruchy wprowadzamy za pomocą myszy

==================================================
Sposób kompilacji:
Serwer uruchamiamy w systemie Linux:
gcc server.c -o server -pthread
./server

Klienta uruchamiamy klikając w plik client.py lub uruchamiając go w terminalu.
Adres IP oraz port wpisujemy w oknie po uruchomieniu programu.