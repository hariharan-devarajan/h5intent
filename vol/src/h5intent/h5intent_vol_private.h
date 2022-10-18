/*-------------------------------------------------------------------------
*
* Created: h5intent_vol_private.h
* Oct 2022
* Hariharan Devarajan <hariharandev1@llnl.gov>
*
* Purpose:Defines the H5Intent VOL Plugin Private Header file
*
*-------------------------------------------------------------------------
*/

#ifndef H5INTENT_VOL_PRIVATE_H
#define H5INTENT_VOL_PRIVATE_H
#include <hdf5.h>
#include <H5VLpublic.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <h5intent/h5intent_vol.h>

#define H5INTENT_VOL_NAME "h5intent"

#define H5INTENT_VOL (h5intent_register())
#ifdef __cplusplus
extern "C"
{
#endif

H5_DLL hid_t h5intent_register(void);

#ifdef __cplusplus
}
#endif
typedef struct H5VL_t {
    const H5VL_class_t *vol_cls;        /* constant driver class info                           */
    /* XXX: Is an integer big enough? */
    int                 nrefs;          /* number of references by objects using this struct    */
    hid_t               vol_id;         /* identifier for the VOL class                         */
} H5VL_t;
typedef struct H5VL_object_t {
    void               *vol_obj;        /* pointer to object created by driver                  */
    H5VL_t             *vol_info;       /* pointer to VOL info struct                           */
} H5VL_object_t;

typedef struct h5intent_wrap_ctx_t
{
    hid_t under_vol_id;   /* VOL ID for under VOL */
    void *under_wrap_ctx; /* Object wrapping context for under VOL */
} H5Intent_wrap_ctx_t;

/* Pass-through VOL connector info */
typedef struct h5intent_info_t
{
    hid_t under_vol_id;   /* VOL ID for under VOL */
    void *under_vol_info; /* VOL info for under VOL */
} H5Intent_info_t;

/**
 * This is the Data structure used within H5intent VOl for maintaining basic data.
 */
typedef struct H5IntentVol {
    hid_t object_id;
    hid_t vol_id;
    hid_t native_id;
    hid_t native_fapl;
    hid_t h5intent_id;
    hid_t h5intent_fapl;
    char* file_name;
    char* dataset_name;
    char* group_name;
    hid_t under_vol_id; /* ID for underlying VOL connector */
    void *under_object; /* Info object for underlying VOL connector */
    bool sync;
} H5Intent_t;

/* H5Intent VOL Dataset callbacks */
static void  *h5intent_dataset_create(void *_item, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
static void *h5intent_dataset_open(void *_item, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
static herr_t h5intent_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t dxpl_id, void *buf, void **req);
static herr_t h5intent_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t dxpl_id, const void *buf, void **req);
static herr_t  h5intent_dataset_get(void *dset, H5VL_dataset_get_args_t *args, hid_t dxpl_id, void **req);
static herr_t h5intent_dataset_optional(void *obj, H5VL_optional_args_t  *args,hid_t dxpl_id, void **req);
static herr_t h5intent_dataset_specific(void *obj, H5VL_dataset_specific_args_t *args, hid_t dxpl_id, void **req);
static herr_t h5intent_dataset_close(void *dset, hid_t dxpl_id, void **req);


/* H5Intent VOL File callbacks */
static void  *h5intent_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
static void  *h5intent_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
static herr_t h5intent_file_get(void *file, H5VL_file_get_args_t *args, hid_t dxpl_id, void **req);
static herr_t h5intent_file_specific_reissue(void *obj, hid_t connector_id, H5VL_file_specific_args_t *args, hid_t dxpl_id, void **req);
static herr_t h5intent_file_specific(void *file, H5VL_file_specific_args_t *args, hid_t dxpl_id, void **req);
static herr_t h5intent_file_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id,
                                     void **req);
static herr_t h5intent_file_close(void *file, hid_t dxpl_id, void **req);

/* H5Intent VOL attribute callbacks*/
static void * h5intent_attribute_create(void *_item, const H5VL_loc_params_t *loc_params, const char *name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req);
static void * h5intent_attribute_open(void *_item, const H5VL_loc_params_t *loc_params, const char *name, hid_t aapl_id, hid_t dxpl_id, void **req);
static herr_t h5intent_attribute_read(void *_attr, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req);
static herr_t h5intent_attribute_write(void *_attr, hid_t mem_type_id, const void *buf,  hid_t dxpl_id, void **req);
static herr_t h5intent_attribute_get(void *_attr, H5VL_attr_get_args_t *args, hid_t dxpl_id, void **req);
static herr_t h5intent_attribute_specific(void *_item, const H5VL_loc_params_t *loc_params, H5VL_attr_specific_args_t *args, hid_t dxpl_id, void **req);
static herr_t h5intent_attribute_close(void *_attr, hid_t dxpl_id, void **req);

/* "Management" callbacks */
static herr_t h5intent_init(hid_t vipl_id);

static herr_t h5intent_term();
void *h5intent_info_copy(const void *info);
herr_t h5intent_info_cmp(int *cmp_value, const void *info1, const void *info2);
herr_t h5intent_info_free(void *info);
herr_t h5intent_info_to_str(const void *info, char **str);
herr_t h5intent_str_to_info(const char *str, void **info);
void *h5intent_get_object(const void *obj);
herr_t h5intent_get_wrap_ctx(const void *obj, void **wrap_ctx);
herr_t h5intent_free_wrap_ctx(void *obj);
void *h5intent_wrap_object(void *obj, H5I_type_t obj_type, void *wrap_ctx);

/* Group callbacks */
void *h5intent_group_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req);
void *h5intent_group_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t gapl_id, hid_t dxpl_id, void **req);
herr_t h5intent_group_get(void *obj, H5VL_group_get_args_t *args, hid_t dxpl_id, void **req);
herr_t h5intent_group_specific(void *obj, H5VL_group_specific_args_t *args, hid_t dxpl_id, void **req);
herr_t h5intent_group_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id,
                               void **req);
herr_t h5intent_group_close(void *grp, hid_t dxpl_id, void **req);


/* definition of H5Intent VOL plugin. */
static const H5VL_class_t H5Intent_g = {
        1,                                      /* Plugin Version number */
        (H5VL_class_value_t)(H5INTENT),         /* Plugin Value */
        H5INTENT_VOL_NAME,                      /* Plugin Name */
        0,                                      /* Plugin capability flags */
        0,
        h5intent_init,                          /* Plugin initialize */
        h5intent_term,                          /* Plugin terminate */
        {
                sizeof(H5Intent_t),             /* Plugin Info size */
                h5intent_info_copy,             /* Plugin Info copy */
                h5intent_info_cmp,              /* Plugin Info compare */
                h5intent_info_free,             /* Plugin Info free */
                h5intent_info_to_str,           /* Plugin Info To String */
                h5intent_str_to_info,           /* Plugin String To Info */
        },
        {
                h5intent_get_object,            /* Plugin Get Object */
                h5intent_get_wrap_ctx,          /* Plugin Get Wrap Ctx */
                h5intent_wrap_object,           /* Plugin Wrap Object */
                NULL,                           /* Plugin Unwrap Object */
                h5intent_free_wrap_ctx,         /* Plugin Free Wrap Ctx */
        },
        {                                       /* Plugin Attribute cls */
                h5intent_attribute_create,      /* Plugin Attribute create */
                h5intent_attribute_open,        /* Plugin Attribute open */
                h5intent_attribute_read,        /* Plugin Attribute read */
                h5intent_attribute_write,       /* Plugin Attribute write */
                h5intent_attribute_get,         /* Plugin Attribute get */
                h5intent_attribute_specific,    /* Plugin Attribute specific */
                NULL,                           /* Plugin Attribute optional */
                h5intent_attribute_close,       /* Plugin Attribute close */
        },
        {                                        /* Plugin Dataset cls */
                h5intent_dataset_create,         /* Plugin Dataset create */
                h5intent_dataset_open,           /* Plugin Dataset open */
                h5intent_dataset_read,           /* Plugin Dataset read */
                h5intent_dataset_write,          /* Plugin Dataset write */
                h5intent_dataset_get,            /* Plugin Dataset get */
                h5intent_dataset_specific,       /* Plugin Dataset specific */
                h5intent_dataset_optional,       /* Plugin Dataset optional */
                h5intent_dataset_close           /* Plugin Dataset close */
        },
        {                                        /* Plugin Datatype cls */
                NULL,                            /* Plugin Datatype commit */
                NULL,                            /* Plugin Datatype open */
                NULL,                            /* Plugin Datatype get */
                NULL,                            /* Plugin Datatype specific */
                NULL,                            /* Plugin Datatype optional */
                NULL                             /* Plugin Datatype close */
        },
        {                                        /* Plugin File cls */
                h5intent_file_create,            /* Plugin File create */
                h5intent_file_open,              /* Plugin File open */
                h5intent_file_get,               /* Plugin File get */
                h5intent_file_specific,          /* Plugin File specific */
                h5intent_file_optional,          /* Plugin File optional */
                h5intent_file_close              /* Plugin File close */
        },
        {                                        /* Plugin Group cls */
                h5intent_group_create,           /* Plugin Group create */
                h5intent_group_open,             /* Plugin Group open */
                h5intent_group_get,              /* Plugin Group get */
                h5intent_group_specific,         /* Plugin Group specific */
                h5intent_group_optional,         /* Plugin Group optional */
                h5intent_group_close             /* Plugin Group close */
        },
        {                                        /* Plugin Link cls */
                NULL,                            /* Plugin Link create */
                NULL,                            /* Plugin Link copy */
                NULL,                            /* Plugin Link move */
                NULL,                            /* Plugin Link get */
                NULL,                            /* Plugin Link specific */
                NULL                             /* Plugin Link optional */
        },
        {                                        /* Plugin Object cls */
                NULL,                            /* Plugin Object open */
                NULL,                            /* Plugin Object copy */
                NULL,                            /* Plugin Object get */
                NULL,                            /* Plugin Object specific */
                NULL                             /* Plugin Object optional */
        },
        {
                NULL,                            /* Plugin Request wait */
                NULL,                            /* Plugin Request notify */
                NULL,                            /* Plugin Request cancel */
        },
        {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            },                                     /* Plugin optional */
        {
            NULL,
            NULL,
            NULL,
            NULL
            },
        {
            NULL,
            NULL,
            NULL
            },
        NULL
};

#endif //H5INTENT_VOL_PRIVATE_H