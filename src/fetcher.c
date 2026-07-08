#include "fetcher.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

typedef struct write_buffer {
    char* data;
    size_t size;
} write_buffer_t;

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t chunk_size = size * nmemb;
    write_buffer_t* buf = (write_buffer_t*)userp;

    char* new_data = (char*)realloc(buf->data, buf->size + chunk_size + 1);
    if (new_data == NULL) {
        return 0; /* signalizira grešku libcurl-u */
    }

    buf->data = new_data;
    memcpy(buf->data + buf->size, contents, chunk_size);
    buf->size += chunk_size;
    buf->data[buf->size] = '\0';
    return chunk_size;
}

int fetcher_global_init(void)
{
    return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK ? 0 : -1;
}

void fetcher_global_cleanup(void)
{
    curl_global_cleanup();
}

int fetcher_get(const char* url, unsigned int timeout_seconds, fetch_result_t* out)
{
    memset(out, 0, sizeof(*out));

    CURL* curl = curl_easy_init();
    if (curl == NULL) {
        return -1;
    }

    write_buffer_t buf = { .data = NULL, .size = 0 };

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "konkurentni-web-crawler/1.0");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        free(buf.data);
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out->http_status);

    char* ctype = NULL;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ctype);
    out->content_type = ctype != NULL ? strdup(ctype) : NULL;

    curl_easy_cleanup(curl);

    out->body = buf.data != NULL ? buf.data : strdup("");
    out->size = buf.size;
    return 0;
}

void fetch_result_free(fetch_result_t* result)
{
    free(result->body);
    free(result->content_type);
    result->body = NULL;
    result->content_type = NULL;
}
