# PSI 26Z Zespół 32

## Zadanie 1B
Aby uruchomić serwer oraz klienta należy wejść do folderu `/psi-z32` oraz uruchomić skrypt `run.sh`

```shell
cd psi-z32
./run.sh
```

Docker compose uruchomi kontenery serwera i klienta, po zakończeniu działania jednego z nich, kontenery zostaną automatycznie zatrzymane.
Program kliencki na koniec swojego działania wypisuje podsumowanie.
Aby zmodyfikować rozmiar bufora programu klienta należy przekazać jego rozmiar jako 3 argument pola `command` w serwisie klienta w pliku `docker-compose.yml`. W przypadku braku podania tej wartości, program przyjmie domyślny rozmiar 10KB.

`docker-compose.yml`
```yaml
name: z32_zad1b

services:
  server:
    build: ./server
    image: z32_zad1b_server
    container_name: z32_zad1b_server
    command: ["5000"]
    networks:
      z32net:
        ipv4_address: 172.21.32.10

  client:
    build: ./client
    image: z32_zad1b_client
    container_name: z32_zad1b_client
    command: ["172.21.32.10", "5000", "100"]
    depends_on: [server]
    networks:
      z32net:
        ipv4_address: 172.21.32.11

networks:
  z32net:
    external: true
    name: z32_network

```

Aby zatrzymać kontenery można użyć skryptu `stop.sh` również z folderu `/psi-z32`

```shell
./stop.sh
```