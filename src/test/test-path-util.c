/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2013 Zbigniew Jędrzejewski-Szmek

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <stdio.h>
#include <unistd.h>

#include "path-util.h"
#include "util.h"
#include "macro.h"
#include "strv.h"

#define test_path_compare(a, b, result) {                 \
                assert_se(path_compare(a, b) == result);  \
                assert_se(path_compare(b, a) == -result); \
                assert_se(path_equal(a, b) == !result);   \
                assert_se(path_equal(b, a) == !result);   \
        }

static void test_path(void) {
        test_path_compare("/goo", "/goo", 0);
        test_path_compare("/goo", "/goo", 0);
        test_path_compare("//goo", "/goo", 0);
        test_path_compare("//goo/////", "/goo", 0);
        test_path_compare("goo/////", "goo", 0);

        test_path_compare("/goo/boo", "/goo//boo", 0);
        test_path_compare("//goo/boo", "/goo/boo//", 0);

        test_path_compare("/", "///", 0);

        test_path_compare("/x", "x/", 1);
        test_path_compare("x/", "/", -1);

        test_path_compare("/x/./y", "x/y", 1);
        test_path_compare("x/.y", "x/y", -1);

        test_path_compare("foo", "/foo", -1);
        test_path_compare("/foo", "/foo/bar", -1);
        test_path_compare("/foo/aaa", "/foo/b", -1);
        test_path_compare("/foo/aaa", "/foo/b/a", -1);
        test_path_compare("/foo/a", "/foo/aaa", -1);
        test_path_compare("/foo/a/b", "/foo/aaa", -1);

        assert_se(path_is_absolute("/"));
        assert_se(!path_is_absolute("./"));

        assert_se(is_path("/dir"));
        assert_se(is_path("a/b"));
        assert_se(!is_path("."));

        assert_se(streq(basename("./aa/bb/../file.da."), "file.da."));
        assert_se(streq(basename("/aa///.file"), ".file"));
        assert_se(streq(basename("/aa///file..."), "file..."));
        assert_se(streq(basename("file.../"), ""));

#define test_parent(x, y) {                                \
                _cleanup_free_ char *z = NULL;             \
                int r = path_get_parent(x, &z);            \
                printf("expected: %s\n", y ? y : "error"); \
                printf("actual: %s\n", r<0 ? "error" : z); \
                assert_se((y==NULL) ^ (r==0));             \
                assert_se(y==NULL || path_equal(z, y));    \
        }

        test_parent("./aa/bb/../file.da.", "./aa/bb/..");
        test_parent("/aa///.file", "/aa///");
        test_parent("/aa///file...", "/aa///");
        test_parent("file.../", NULL);

        assert_se(path_is_mount_point("/", true));
        assert_se(path_is_mount_point("/", false));

        {
                char p1[] = "aaa/bbb////ccc";
                char p2[] = "//aaa/.////ccc";
                char p3[] = "/./";

                assert_se(path_equal(path_kill_slashes(p1), "aaa/bbb/ccc"));
                assert_se(path_equal(path_kill_slashes(p2), "/aaa/./ccc"));
                assert_se(path_equal(path_kill_slashes(p3), "/./"));
        }
}

static void test_find_binary(const char *self, bool local) {
        char *p;

        assert_se(find_binary("/bin/sh", local, &p) == 0);
        puts(p);
        assert_se(streq(p, "/bin/sh"));
        free(p);

        assert_se(find_binary(self, local, &p) == 0);
        puts(p);
        assert_se(endswith(p, "/test-path-util"));
        assert_se(path_is_absolute(p));
        free(p);

        assert_se(find_binary("sh", local, &p) == 0);
        puts(p);
        assert_se(endswith(p, "/sh"));
        assert_se(path_is_absolute(p));
        free(p);

        assert_se(find_binary("xxxx-xxxx", local, &p) == -ENOENT);

        assert_se(find_binary("/some/dir/xxxx-xxxx", local, &p) ==
                  (local ? -ENOENT : 0));
        if (!local)
                free(p);
}

static void test_prefixes(void) {
        static const char* values[] = { "/a/b/c/d", "/a/b/c", "/a/b", "/a", "", NULL};
        unsigned i;
        char s[PATH_MAX];
        bool b;

        i = 0;
        PATH_FOREACH_PREFIX_MORE(s, "/a/b/c/d") {
                log_error("---%s---", s);
                assert_se(streq(s, values[i++]));
        }
        assert_se(values[i] == NULL);

        i = 1;
        PATH_FOREACH_PREFIX(s, "/a/b/c/d") {
                log_error("---%s---", s);
                assert_se(streq(s, values[i++]));
        }
        assert_se(values[i] == NULL);

        i = 0;
        PATH_FOREACH_PREFIX_MORE(s, "////a////b////c///d///////")
                assert_se(streq(s, values[i++]));
        assert_se(values[i] == NULL);

        i = 1;
        PATH_FOREACH_PREFIX(s, "////a////b////c///d///////")
                assert_se(streq(s, values[i++]));
        assert_se(values[i] == NULL);

        PATH_FOREACH_PREFIX(s, "////")
                assert_not_reached("Wut?");

        b = false;
        PATH_FOREACH_PREFIX_MORE(s, "////") {
                assert_se(!b);
                assert_se(streq(s, ""));
                b = true;
        }
        assert_se(b);

        PATH_FOREACH_PREFIX(s, "")
                assert_not_reached("wut?");

        b = false;
        PATH_FOREACH_PREFIX_MORE(s, "") {
                assert_se(!b);
                assert_se(streq(s, ""));
                b = true;
        }
}

static void test_path_join(void) {

#define test_join(root, path, rest, expected) {  \
                _cleanup_free_ char *z = NULL;   \
                z = path_join(root, path, rest); \
                assert_se(streq(z, expected));   \
        }

        test_join("/root", "/a/b", "/c", "/root/a/b/c");
        test_join("/root", "a/b", "c", "/root/a/b/c");
        test_join("/root", "/a/b", "c", "/root/a/b/c");
        test_join("/root", "/", "c", "/root/c");
        test_join("/root", "/", NULL, "/root/");

        test_join(NULL, "/a/b", "/c", "/a/b/c");
        test_join(NULL, "a/b", "c", "a/b/c");
        test_join(NULL, "/a/b", "c", "/a/b/c");
        test_join(NULL, "/", "c", "/c");
        test_join(NULL, "/", NULL, "/");
}

static void test_fsck_exists(void) {
        /* Ensure we use a sane default for PATH. */
        unsetenv("PATH");

        /* fsck.minix is provided by util-linux and will probably exist. */
        assert_se(fsck_exists("minix") == 0);

        assert_se(fsck_exists("AbCdE") == -ENOENT);
}

static void test_make_relative(void) {
        char *result;

        assert_se(path_make_relative("some/relative/path", "/some/path", &result) < 0);
        assert_se(path_make_relative("/some/path", "some/relative/path", &result) < 0);

#define test(from_dir, to_path, expected) {                \
                _cleanup_free_ char *z = NULL;             \
                path_make_relative(from_dir, to_path, &z); \
                assert_se(streq(z, expected));             \
        }

        test("/", "/", ".");
        test("/", "/some/path", "some/path");
        test("/some/path", "/some/path", ".");
        test("/some/path", "/some/path/in/subdir", "in/subdir");
        test("/some/path", "/", "../..");
        test("/some/path", "/some/other/path", "../other/path");
        test("//extra/////slashes///won't////fool///anybody//", "////extra///slashes////are/just///fine///", "../../../are/just/fine");
}

static void test_strv_resolve(void) {
        char tmp_dir[] = "/tmp/test-path-util-XXXXXX";
        _cleanup_strv_free_ char **search_dirs = NULL;
        _cleanup_strv_free_ char **absolute_dirs = NULL;
        char **d;

        assert_se(mkdtemp(tmp_dir) != NULL);

        search_dirs = strv_new("/dir1", "/dir2", "/dir3", NULL);
        assert_se(search_dirs);
        STRV_FOREACH(d, search_dirs) {
                char *p = strappend(tmp_dir, *d);
                assert_se(p);
                assert_se(strv_push(&absolute_dirs, p) == 0);
        }

        assert_se(mkdir(absolute_dirs[0], 0700) == 0);
        assert_se(mkdir(absolute_dirs[1], 0700) == 0);
        assert_se(symlink("dir2", absolute_dirs[2]) == 0);

        path_strv_resolve(search_dirs, tmp_dir);
        assert_se(streq(search_dirs[0], "/dir1"));
        assert_se(streq(search_dirs[1], "/dir2"));
        assert_se(streq(search_dirs[2], "/dir2"));

        assert_se(rm_rf_dangerous(tmp_dir, false, true, false) == 0);
}

static void test_path_startswith(void) {
        assert_se(path_startswith("/foo/bar/barfoo/", "/foo"));
        assert_se(path_startswith("/foo/bar/barfoo/", "/foo/"));
        assert_se(path_startswith("/foo/bar/barfoo/", "/"));
        assert_se(path_startswith("/foo/bar/barfoo/", "////"));
        assert_se(path_startswith("/foo/bar/barfoo/", "/foo//bar/////barfoo///"));
        assert_se(path_startswith("/foo/bar/barfoo/", "/foo/bar/barfoo////"));
        assert_se(path_startswith("/foo/bar/barfoo/", "/foo/bar///barfoo/"));
        assert_se(path_startswith("/foo/bar/barfoo/", "/foo////bar/barfoo/"));
        assert_se(path_startswith("/foo/bar/barfoo/", "////foo/bar/barfoo/"));
        assert_se(path_startswith("/foo/bar/barfoo/", "/foo/bar/barfoo"));

        assert_se(!path_startswith("/foo/bar/barfoo/", "/foo/bar/barfooa/"));
        assert_se(!path_startswith("/foo/bar/barfoo/", "/foo/bar/barfooa"));
        assert_se(!path_startswith("/foo/bar/barfoo/", ""));
        assert_se(!path_startswith("/foo/bar/barfoo/", "/bar/foo"));
        assert_se(!path_startswith("/foo/bar/barfoo/", "/f/b/b/"));
}

int main(int argc, char **argv) {
        test_path();
        test_find_binary(argv[0], true);
        test_find_binary(argv[0], false);
        test_prefixes();
        test_path_join();
        test_fsck_exists();
        test_make_relative();
        test_strv_resolve();
        test_path_startswith();

        return 0;
}
