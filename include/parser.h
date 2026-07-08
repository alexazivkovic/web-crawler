#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

/*
 * Jednostavan parser linkova. Ovo NIJE potpun HTML/DOM parser - to nije
 * cilj ovog projekta (fokus je na konkurentnosti). Umesto toga, parser
 * skenira sirovi HTML tekst i traži sve "href" atribute (npr. iz <a>, ali
 * i iz drugih tagova koji ga sadrže), izvlači njihovu vrednost i svodi je
 * na apsolutan URL u odnosu na baznu stranicu sa koje je preuzeta.
 *
 * Linkovi sa nepodržanim šemama (mailto:, javascript:, tel:, data: ...),
 * kao i čisti fragmenti (#anchor), se preskaču.
 */

/*
 * Izvlači sve href linkove iz html sadržaja i razrešava ih u apsolutne
 * URL-ove u odnosu na base_url. Alocira niz stringova (*out_links) koji
 * pozivalac mora osloboditi pozivom parser_free_links().
 * Vraća 0 na uspeh (čak i ako je pronađeno 0 linkova), -1 na grešku alokacije.
 */
int parser_extract_links(const char* html, const char* base_url,
    char*** out_links, size_t* out_count);

void parser_free_links(char** links, size_t count);

/*
 * Razrešava (potencijalno relativan) link u odnosu na base_url i upisuje
 * apsolutan URL u out (veličine out_size). Vraća 1 ako je link podržan
 * i uspešno razrešen, 0 ako treba preskočiti (fragment, mailto:, itd).
 */
int parser_resolve_url(const char* base_url, const char* link, char* out, size_t out_size);

#endif
