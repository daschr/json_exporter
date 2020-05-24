#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <jansson.h>
#include <prom.h>
#include <promhttp.h>
#include <microhttpd.h>
#include "uthash.h"

struct metric_elem {
    char *name;
    prom_gauge_t *gauge;
    UT_hash_handle hh;
};


// gc collectable
struct metric_elem *ltable;
json_t *pjson;
struct MHD_Daemon *web_daemon;


void cleanup_table() {
    struct metric_elem *c, *tmp;

    HASH_ITER(hh, ltable, c, tmp) {
        HASH_DEL(ltable, c);
        free(c->name);
        free(c);
    }
}

void sig_handler(int e) {
    if(pjson != NULL)
        json_decref(pjson);
    prom_collector_registry_destroy(PROM_COLLECTOR_REGISTRY_DEFAULT);
    MHD_stop_daemon(web_daemon);
    cleanup_table();
    exit(e);
}


int main(int argc, char *argv[]) {
    prom_collector_registry_default_init();
    promhttp_set_active_collector_registry(NULL);
    int port=8000;
    if(argc > 1)
        sscanf(argv[1], "%d", &port);

    struct metric_elem *tmp_tab=NULL;
    json_error_t jerror;

    web_daemon=promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, port, NULL, NULL);
    if(daemon == NULL)
        sig_handler(EXIT_FAILURE);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGSEGV, sig_handler);

    while((pjson=json_loadf(stdin, JSON_DISABLE_EOF_CHECK, &jerror)) != NULL) {
        double nval;
        const char *key;
        json_t *val;
        json_object_foreach(pjson, key, val) {
            if(json_is_number(val)) {
                nval=json_number_value(val);

                HASH_FIND_STR(ltable, key, tmp_tab);
                if(tmp_tab == NULL) {
                    tmp_tab=malloc(sizeof(struct metric_elem));
                    tmp_tab->name=strdup(key);
                    tmp_tab->gauge=prom_collector_registry_must_register_metric(prom_gauge_new(tmp_tab->name, "", 0, NULL));

                    HASH_ADD_KEYPTR(hh, ltable, tmp_tab->name, strlen(tmp_tab->name), tmp_tab);
                }
#ifdef DEBUG
                printf("gauge \"%s\" gets value %lf\n", key, nval);
#endif
                prom_gauge_set(tmp_tab->gauge, nval, NULL);
            }
        }
        json_decref(pjson);
    }


    if(json_error_code(&jerror) != json_error_premature_end_of_input)
        fprintf(stderr, "%s\n", jerror.text);

    sig_handler(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}
