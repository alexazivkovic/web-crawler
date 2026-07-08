#include "parser.h"

#include <criterion/criterion.h>
#include <criterion/new/assert.h>

Test(parser, resolves_absolute_relative_and_root_relative_links)
{
    char out[256];

    cr_assert(eq(int, parser_resolve_url("https://example.com/blog/post.html",
                     "https://other.com/x", out, sizeof(out)), 1));
    cr_assert(eq(str, out, "https://other.com/x"));

    cr_assert(eq(int, parser_resolve_url("https://example.com/blog/post.html",
                     "/about", out, sizeof(out)), 1));
    cr_assert(eq(str, out, "https://example.com/about"));

    cr_assert(eq(int, parser_resolve_url("https://example.com/blog/post.html",
                     "next.html", out, sizeof(out)), 1));
    cr_assert(eq(str, out, "https://example.com/blog/next.html"));

    cr_assert(eq(int, parser_resolve_url("https://example.com/blog/post.html",
                     "//cdn.example.com/lib.js", out, sizeof(out)), 1));
    cr_assert(eq(str, out, "https://cdn.example.com/lib.js"));
}

Test(parser, skips_fragments_and_unsupported_schemes)
{
    char out[256];
    const char* base = "https://example.com/blog/post.html";

    cr_assert(eq(int, parser_resolve_url(base, "#section2", out, sizeof(out)), 0));
    cr_assert(eq(int, parser_resolve_url(base, "mailto:a@b.com", out, sizeof(out)), 0));
    cr_assert(eq(int, parser_resolve_url(base, "javascript:void(0)", out, sizeof(out)), 0));
    cr_assert(eq(int, parser_resolve_url(base, "ftp://x.example/file", out, sizeof(out)), 0));
}

Test(parser, extracts_multiple_links_from_html)
{
    const char* html =
        "<html><body>"
        "<a href=\"/page1\">One</a>"
        "<a href='https://external.example/page2'>Two</a>"
        "<a href=\"#skip-me\">Skip</a>"
        "<link href=\"style.css\">"
        "</body></html>";

    char** links = NULL;
    size_t count = 0;

    cr_assert(eq(int, parser_extract_links(html, "https://example.com/dir/index.html",
                     &links, &count), 0));
    cr_assert(eq(u64, (uint64_t)count, (uint64_t)3));
    cr_assert(eq(str, links[0], "https://example.com/page1"));
    cr_assert(eq(str, links[1], "https://external.example/page2"));
    cr_assert(eq(str, links[2], "https://example.com/dir/style.css"));

    parser_free_links(links, count);
}
