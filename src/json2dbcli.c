/**
 * @file      json2dbcli.c
 * 
 * @version   0.1.0
 *
 * @date      17-09-2026
 *  
 * @author    Fábio D. Pacheco, 
 * @email     fabio.d.pacheco@inesctec.pt or pacheco.castro.fabio@gmail.com
 * 
 * @note
 * sibdb 
 * Copyright (C) 2026 Fábio D. Pacheco 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 * 
 */

#define PRGNAME "json2dbcli"
#define STB_JSON2DB_IMPLEMENTATION

#include "stb_json2db.h"
#include "stblog.h"

#define HELP_MSG \
        "Usage:\n" \
        "  " PRGNAME " \"target_table\" \"timestamp\" '{\"json\": \"payload\"}'\n\n" \
        "Arguments:\n" \
        "  <operation>    : --create|--add|--clean" \
        "  <target_table> : Target PostgreSQL table name\n" \
        "  <timestamp>    : ISO-8601 timestamp (e.g. \"YYYY-MM-DD HH:MM:SS±HH:MM\") or \"now\"\n" \
        "  <json_payload> : Valid JSON payload string containing sensor key-value pairs\n"

static const char* 
get_env_default(
        const char *env_var, 
        const char *def_val
) {
        const char *val = getenv(env_var);
        return (val && val[0] != '\0') ? val : def_val;
}

int 
main(
        int argc, 
        char **argv
) {
        struct pgctx ctx;
        int err = EXIT_FAILURE;

        if (argc < 2) {
                err = -EINVAL;
                LOG_ERRNO(0, PRGNAME, err, LOG_WARN, "\n%s", HELP_MSG);
                return EXIT_FAILURE;
        }
        err = json2db_init(
                &ctx,
                get_env_default("PGHOST", "127.0.0.1"), 
                get_env_default("PGPORT", "5432"), 
                get_env_default("DB_NAME", "sib"), 
                get_env_default("DB_USER", "creator"),
                get_env_default("DB_PASSWORD", "pswd")
        );
        if (err) {
                LOG_ERRNO(0, PRGNAME, err, LOG_WARN, "DB Connection failed: %s", PQerrorMessage(ctx.conn));
                goto cleanup;
        }
        const char *op = argv[1];
        if (!strcmp(op, "--add")) {
                if (argc < 5) {
                        err = -EINVAL;
                        LOG_ERRNO(0, PRGNAME, err, LOG_WARN, "\n%s", HELP_MSG);
                        return EXIT_FAILURE;
                }
                const char *tabname   = argv[2];
                const char *timestamp = argv[3];
                const char *json      = argv[4];               
                if (!strlen(tabname) || !strlen(json)) {
                        err = -EINVAL;
                        LOG_ERRNO(0, PRGNAME, err, LOG_WARN, "Target table and JSON payload cannot be empty.");
                        return EXIT_FAILURE;
                }
                err = json2db_add(&ctx, tabname, timestamp, json);
                if (err) {
                        LOG_ERRNO(0, PRGNAME, err, LOG_WARN, "Execution failed: %s", PQerrorMessage(ctx.conn));
                        goto cleanup;
                }
        }
        else if (!strcmp(op, "--create")) {
                err = json2db_new(&ctx);
                if (err) {
                        LOG_ERRNO(0, PRGNAME, err, LOG_WARN, "Execution failed: %s", PQerrorMessage(ctx.conn));
                        goto cleanup;
                }
        }
        else if (!strcmp(op, "--clean")) {
                if (argc < 3) {
                        err = -EINVAL;
                        LOG_ERRNO(0, PRGNAME, err, LOG_WARN, "\n%s", HELP_MSG);
                        return EXIT_FAILURE;
                }
                const char *tabname   = argv[2];
                err = json2db_clean(&ctx, tabname);
                if (err) {
                        LOG_ERRNO(0, PRGNAME, err, LOG_WARN, "Execution failed: %s", PQerrorMessage(ctx.conn));
                        goto cleanup;
                }
        }
        err = EXIT_SUCCESS;
cleanup:
        return json2db_deinit(&ctx);
}
