# Konkurentni web crawler

Konkurentni (multi-threaded) web crawler napisan u C-u, sa **deljenim redom
URL-ova** i **deljenim skupom posećenih stranica**. Projekat je rađen u
okviru predmeta Konkurentno i distribuirano programiranje, sa fokusom na bezbednu
sinhronizaciju pristupa deljenim strukturama podataka iz više thread-ova
(mutex + condition variable), a ne na naprednu obradu HTML-a.

Detaljna dokumentacija projekta nalazi se u `Dokumentacija.docx`.

## Kako radi (ukratko)

- Glavna nit ubacuje **seed URL(ove)** u deljeni red i pokreće **pool worker
  thread-ova** (veličina podesiva parametrom `-t`).
- Svaki worker u petlji: skida URL sa reda → preuzima stranicu (libcurl) →
  izvlači `href` linkove iz HTML-a → za svaki novi link proverava/upisuje
  ga u deljeni skup posećenih stranica (atomično, bez duplog obilaska) →
  nove linkove ubacuje nazad u red.
- Kada nema više posla (red prazan i nijedan worker ne obrađuje ništa),
  svi thread-ovi se gase (graceful shutdown) i program ispisuje statistiku.
- Dubina obilaska (`-d`) i broj stranica (`-p`) su ograničeni da bi crawl
  bio deterministički i vremenski ograničen; dodatno postoji i "watchdog"
  koji prisilno gasi program ako iz bilo kog razloga (npr. bug u
  sinhronizaciji) predugo traje.

## Struktura projekta

```
include/
  url_queue.h      - API deljenog reda URL-ova (mutex + cond var)
  visited_set.h    - API deljenog hash skupa posećenih URL-ova
  fetcher.h        - API za HTTP preuzimanje stranica (libcurl)
  parser.h         - API za izvlačenje i razrešavanje linkova iz HTML-a
  crawler.h        - orkestracija: konfiguracija, statistika, thread pool
  utils/watchdog.h - sigurnosni tajmer koji gasi proces posle timeout-a

src/
  url_queue.c
  visited_set.c
  fetcher.c
  parser.c
  crawler.c
  main.c           - CLI, parsiranje argumenata, pokretanje crawl-a
  utils/watchdog.c

tests/
  test_url_queue.c    - FIFO redosled, graceful shutdown, konkurentni producer-i
  test_visited_set.c  - dedup logika, race izmedju thread-ova na istom URL-u
  test_parser.c       - izvlačenje linkova, razrešavanje relativnih URL-ova
  test_main.c         - primer/sanity test
```

## Zavisnosti

- CMake >= 3.10
- C kompajler sa podrškom za C99 i POSIX thread-ove (pthread)
- **libcurl** (razvojni paket, npr. `libcurl4-openssl-dev` na Debian/Ubuntu,
  ili `brew install curl` na macOS - na macOS je već dostupan preko
  Command Line Tools SDK-a)
- (opciono, samo za testove) **Criterion** test framework
  - macOS: `brew install criterion`
  - Debian/Ubuntu: `sudo apt install libcriterion-dev`
  - Ako Criterion nije instaliran, CMake preskače build testova i ostatak
    projekta se normalno builduje.

## Build i pokretanje

Projekat koristi priloženu `./build` skriptu (wrapper oko CMake-a):

```bash
./build --debug          # build (Debug)
./build --release        # build (Release, sa optimizacijama)
./build --clean --debug  # čist build od nule
./build --run <opcije i seed URL-ovi>   # build (ako treba) i pokreni
```

Primer pokretanja:

```bash
./build --release
./out/Release/konkurentno_template -t 8 -d 2 -p 100 -v https://example.com
```

### CLI opcije

| Opcija | Opis | Podrazumevano |
|---|---|---|
| `-t, --threads N` | broj worker thread-ova | 4 |
| `-d, --max-depth N` | maksimalna dubina praćenja linkova | 2 |
| `-p, --max-pages N` | maksimalan broj preuzetih stranica, `0` = bez limita | 50 |
| `-T, --timeout SEC` | timeout po HTTP zahtevu | 10 |
| `-w, --watchdog SEC` | tvrdi limit trajanja programa, `0` = isključeno | 120 |
| `-v, --verbose` | ispisuje svaku preuzetu stranicu tokom rada | isključeno |
| `-h, --help` | pomoć | - |

Program prima jedan ili više seed URL-ova (npr. za paralelno pretraživanje
više nezavisnih sajtova iz istog reda i skupa).

Na kraju izvršavanja ispisuje se statistika: broj uspešno preuzetih
stranica, broj neuspešnih preuzimanja, broj otkrivenih linkova, ukupan
broj jedinstvenih posećenih URL-ova i vreme izvršavanja.

> Napomena: pošto više worker-a radi paralelno, broj stvarno preuzetih
> stranica pri dostizanju `--max-pages` limita može neznatno preći zadatu
> vrednost (stranice koje su već "u letu" kada je limit dostignut se
> dovršavaju). Ovo je očekivano ponašanje kod konkurentnog obilaska, ne greška.

## Pokretanje testova

```bash
./build --debug
./out/Debug/konkurentno_template_tests --verbose
```

(Testovi se build-uju samo ako je Criterion instaliran - videti sekciju
Zavisnosti.)

## Ograničenja / napomene o obimu

- Parser linkova nije potpun HTML/DOM parser - namerno je pojednostavljen
  (skenira `href` atribute) jer fokus zadatka nije parsiranje HTML-a, već
  konkurentnost i sinhronizacija deljenih struktura.
- Skup posećenih stranica čuva se samo u memoriji (nema perzistencije
  između pokretanja programa).
