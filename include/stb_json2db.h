/**
 * @file      stb_json2db.h
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

#ifndef STB_JSON2SQL_H
#define STB_JSON2SQL_H

#include <postgresql/libpq-fe.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pgctx {
        PGconn *conn;
};

int
json2db_init(
        struct pgctx *ctx,
        const char *host,
        const char *port,
        const char *dbnm,
        const char *user,
        const char *pswd
);

int 
json2db_deinit(
        struct pgctx *ctx
);

int
json2db_new(
        struct pgctx *ctx
);

int
json2db_add(
        struct pgctx *ctx,
        const char   *tabname,
        const char   *timestamp,
        const char   *json
);

int 
json2db_clean(
        struct pgctx *ctx,
        const char   *tabname
);

int 
_json2db_syntaxcheck(
        const char *json
);

#ifdef __cplusplus
}
#endif


#ifdef STB_JSON2DB_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define _TEMPL_FUNC \
        "CREATE OR REPLACE FUNCTION siblog(tabname TEXT, received_at TIMESTAMPTZ, payload JSONB) "\
        "RETURNS VOID AS $$ "\
        "DECLARE "\
        "        key_name TEXT; "\
        "        key_value JSONB; "\
        "        col_type TEXT; "\
        "        col_names TEXT; "\
        "BEGIN "\
        "        EXECUTE format( "\
        "                'CREATE TABLE IF NOT EXISTS %I (id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, received_at TIMESTAMPTZ)',  "\
        "                tabname "\
        "        ); "\
        "        FOR key_name, key_value IN SELECT * FROM jsonb_each(payload) LOOP "\
        "                col_type := CASE  "\
        "                        WHEN jsonb_typeof(key_value) = 'number' THEN 'NUMERIC' "\
        "                        WHEN jsonb_typeof(key_value) = 'boolean' THEN 'BOOLEAN' "\
        "                        WHEN jsonb_typeof(key_value) IN ('object', 'array') THEN 'JSONB' "\
        "                        ELSE 'TEXT' "\
        "                END; "\
        "                EXECUTE format( "\
        "                        'ALTER TABLE %I ADD COLUMN IF NOT EXISTS %I %s',  "\
        "                        tabname, key_name, col_type "\
        "                ); "\
        "        END LOOP; "\
        "        SELECT string_agg(quote_ident(key), ', ') INTO col_names FROM jsonb_each(payload); "\
        "        EXECUTE format( "\
        "                'INSERT INTO %I (received_at, %s) SELECT %L::TIMESTAMPTZ, %s FROM jsonb_populate_record(NULL::%I, %L)',  "\
        "                tabname, col_names, received_at, col_names, tabname, payload "\
        "        ); "\
        "END;  "\
        "$$ LANGUAGE plpgsql; "\

#define _TEMPL_CALL \
        "SELECT siblog($1::text, $2::timestamptz, $3::jsonb);"

#define _TEMPL_CLEAR \
        "TRUNCATE TABLE %s RESTART IDENTITY"

int
json2db_init(
        struct pgctx *ctx,
        const char *host,
        const char *port,
        const char *dbnm,
        const char *user,
        const char *pswd
) {
        if (!ctx || !host || !port || !dbnm || !user || !pswd) {
                return -EINVAL;
        }
        ctx->conn = PQsetdbLogin(host, port, NULL, NULL, dbnm, user, pswd);
        if (CONNECTION_OK != PQstatus(ctx->conn)) {
                return -ECONNREFUSED;
        }
        return 0;
}

int 
_json2db_syntaxcheck(
        const char *json
) {
        if (!json) {
                return -EINVAL;
        }
        int prove = 0;  
        for (unsigned int i=0 ; i<strlen(json) ; ++i) {
                switch(json[i]){
                case '{': prove++; break;
                case '}': if(prove) {return 1;} break;
                }
        }
        return 0;
}

int
json2db_new(
        struct pgctx *ctx
) {
        if (!ctx) {
                return -EINVAL;
        }
        PGresult *res = PQexec(ctx->conn, _TEMPL_FUNC);
        if (PGRES_COMMAND_OK != PQresultStatus(res)) {
                return -EIO;
        }
        if (res) {
                PQclear(res);
        }
        return 0;
}

int 
json2db_clean(
        struct pgctx *ctx,
        const char   *tabname
) {
        if (!ctx || !tabname) {
                return -EINVAL;
        } 
        char query[512];
        char *tab = PQescapeIdentifier(ctx->conn, tabname, strlen(tabname));
        if (!tab) {
                return -EIO;
        }
        snprintf(query, sizeof(query), _TEMPL_CLEAR, tab);
        PQfreemem(tab);
        PGresult *res = PQexec(ctx->conn, query);
        if (PGRES_COMMAND_OK != PQresultStatus(res)) {
                if (res) {
                        PQclear(res);
                }
                return -EIO;
        }
        PQclear(res);
        return 0;
}

int
json2db_add(
        struct pgctx *ctx,
        const char   *tabname,
        const char   *timestamp,
        const char   *json
) {
        if (!ctx || !tabname || !timestamp || !json || !_json2db_syntaxcheck(json)) {
                return -EINVAL;
        } 
        PGresult *res = PQexecParams(
                ctx->conn,
                _TEMPL_CALL,
                3,             
                NULL,          
                (const char *[]){tabname, timestamp, json},
                (const int   []){(int)strlen(tabname), (int)strlen(timestamp), (int)strlen(json)},
                (const int   []){0, 0, 0},
                0
        );
        if (PGRES_TUPLES_OK != PQresultStatus(res) && PGRES_COMMAND_OK != PQresultStatus(res)) {
                return -EIO;
        }
        if (res) {
                PQclear(res);
        }
        return 0;
}

int 
json2db_deinit(
        struct pgctx *ctx
) {
        if (!ctx || !ctx->conn) {
                return 0;
        }
        PQfinish(ctx->conn);
        return 0;
}

#endif

#endif
