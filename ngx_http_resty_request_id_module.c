/*
 * Copyright (C) Hanada
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

typedef struct {
    ngx_str_t resty_request_id;
} ngx_http_resty_request_id_ctx_t;

static ngx_int_t ngx_http_resty_request_id_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_resty_request_id(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


static ngx_http_module_t  ngx_http_resty_request_id_module_ctx = {
    ngx_http_resty_request_id_add_variables,      /* preconfiguration */
    NULL,                                   /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    NULL,                                   /* create location configuration */
    NULL                                    /* merge location configuration */
};


ngx_module_t  ngx_http_resty_request_id_module = {
    NGX_MODULE_V1,
    &ngx_http_resty_request_id_module_ctx,         /* module context */
    NULL,                                    /* module directives */
    NGX_HTTP_MODULE,                         /* module type */
    NULL,                                    /* init master */
    NULL,                                    /* init module */
    NULL,                                    /* init process */
    NULL,                                    /* init thread */
    NULL,                                    /* exit thread */
    NULL,                                    /* exit process */
    NULL,                                    /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_variable_t  ngx_http_resty_request_id_variables[] = {
    { ngx_string("resty_request_id"), NULL, ngx_http_resty_request_id,
      0, 0, NGX_HTTP_VAR_NOCACHEABLE },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


static ngx_int_t
ngx_http_resty_request_id_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_resty_request_id_variables; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_resty_request_id(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                          *id, *p;
    ngx_time_t                      *tp;
    size_t                           len;
    ngx_http_resty_request_id_ctx_t *ctx;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    ctx = ngx_http_get_module_ctx(r, ngx_http_resty_request_id_module);
    if (ctx != NULL && ctx->resty_request_id.len > 0) {
        v->len = ctx->resty_request_id.len;
        v->data = ctx->resty_request_id.data;
        return NGX_OK;
    }

    if (r->main != r) {
        ctx = ngx_http_get_module_ctx(r->main, ngx_http_resty_request_id_module);
        if (ctx != NULL && ctx->resty_request_id.len > 0) {
            v->len = ctx->resty_request_id.len;
            v->data = ctx->resty_request_id.data;
            return NGX_OK;
        }
    }

    if (r->headers_in.x_resty_request_id) {
        v->len = r->headers_in.x_resty_request_id->value.len;
        v->data = r->headers_in.x_resty_request_id->value.data;

    } else {
        len = NGX_TIME_T_LEN + ngx_cycle->hostname.len + NGX_INT64_LEN
            + NGX_ATOMIC_T_LEN + NGX_INT_T_LEN + 4;
        id = ngx_pnalloc(r->pool, len);
        if (id == NULL) {
            return NGX_ERROR;
        }

        tp = ngx_timeofday();
        p = ngx_sprintf(id, "%xT_%*s_%P-%uA-%ui", 
                        tp->sec, ngx_cycle->hostname.len, ngx_cycle->hostname.data,
                        ngx_pid, r->connection->number, r->connection->requests);

        v->len = p - id;
        v->data = id;
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_resty_request_id_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ctx->resty_request_id.len = v->len;
    ctx->resty_request_id.data = v->data;
    ngx_http_set_ctx(r, ctx, ngx_http_resty_request_id_module);

    return NGX_OK;
}