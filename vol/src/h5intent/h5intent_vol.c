/*
 * Purpose:     This is a "intent" VOL connector, which forwards each
 *              VOL callback to an underlying connector.
 *
 *              It is designed as an example VOL connector for developers to
 *              use when creating new connectors, especially connectors that
 *              are outside of the HDF5 library.  As such, it should _NOT_
 *              include _any_ private HDF5 header files.  This connector should
 *              therefore only make public HDF5 API calls and use standard C /
 *              POSIX calls.
 *
 *              Note that the HDF5 error stack must be preserved on code paths
 *              that could be invoked when the underlying VOL connector's
 *              callback can fail.
 *
 */

/* Header files needed */
/* Do NOT include private HDF5 files here! */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cpp-logger/clogger.h>
#define H5_INTENT_LOG_NAME "H5INTENT"
#define H5INTENT_LOGINFO(format, ...) \
  cpp_logger_clog(CPP_LOGGER_INFO, H5_INTENT_LOG_NAME, format, __VA_ARGS__);
#define H5INTENT_LOGINFO_SIMPLE(format) \
  cpp_logger_clog(CPP_LOGGER_INFO, H5_INTENT_LOG_NAME, format);
#define H5INTENT_LOGWARN(format, ...) \
  cpp_logger_clog(CPP_LOGGER_WARN, H5_INTENT_LOG_NAME, format, __VA_ARGS__);
#define H5INTENT_LOGERROR(format, ...) \
  cpp_logger_clog(CPP_LOGGER_ERROR, H5_INTENT_LOG_NAME, format, __VA_ARGS__);
#define H5INTENT_LOGPRINT(format, ...) \
  cpp_logger_clog(CPP_LOGGER_PRINT, H5_INTENT_LOG_NAME, format, __VA_ARGS__);

/* Public HDF5 file */
#include <hdf5.h>

/* This connector's header */
#include <h5intent/h5intent_vol.h>
#include <unistd.h>

#include "h5intent/property_dds.h"

/**********/
/* Macros */
/**********/

/* Whether to display log messge when callback is invoked */
/* (Uncomment to enable) */
/* #define ENABLE_INTENT_LOGGING */

/* Hack for missing va_copy() in old Visual Studio editions
 * (from H5win2_defs.h - used on VS2012 and earlier)
 */
#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER < 1800)
#define va_copy(D, S) ((D) = (S))
#endif

/************/
/* Typedefs */
/************/

/* The intent VOL info object */
typedef struct H5VL_intent_t {
  hid_t under_vol_id; /* ID for underlying VOL connector */
  void *under_object; /* Info object for underlying VOL connector */
} H5VL_intent_t;

/* The intent VOL wrapper context */
typedef struct H5VL_intent_wrap_ctx_t {
  hid_t under_vol_id;   /* VOL ID for under VOL */
  void *under_wrap_ctx; /* Object wrapping context for under VOL */
} H5VL_intent_wrap_ctx_t;

/********************* */
/* Function prototypes */
/********************* */

/* Helper routines */
static herr_t H5VL_intent_file_specific_reissue(void *obj, hid_t connector_id,
                                                H5VL_file_specific_args_t *args,
                                                hid_t dxpl_id, void **req);

static herr_t H5VL_intent_request_specific_reissue(
    void *obj, hid_t connector_id, H5VL_request_specific_args_t *args);

static herr_t H5VL_intent_link_create_reissue(
    H5VL_link_create_args_t *args, void *obj,
    const H5VL_loc_params_t *loc_params, hid_t connector_id, hid_t lcpl_id,
    hid_t lapl_id, hid_t dxpl_id, void **req);

static H5VL_intent_t *H5VL_intent_new_obj(void *under_obj, hid_t under_vol_id);

static herr_t H5VL_intent_free_obj(H5VL_intent_t *obj);

/* "Management" callbacks */
static herr_t H5VL_intent_init(hid_t vipl_id);

static herr_t H5VL_intent_term(void);

/* VOL info callbacks */
static void *H5VL_intent_info_copy(const void *info);

static herr_t H5VL_intent_info_cmp(int *cmp_value, const void *info1,
                                   const void *info2);

static herr_t H5VL_intent_info_free(void *info);

static herr_t H5VL_intent_info_to_str(const void *info, char **str);

static herr_t H5VL_intent_str_to_info(const char *str, void **info);

/* VOL object wrap / retrieval callbacks */
static void *H5VL_intent_get_object(const void *obj);

static herr_t H5VL_intent_get_wrap_ctx(const void *obj, void **wrap_ctx);

static void *H5VL_intent_wrap_object(void *obj, H5I_type_t obj_type,
                                     void *wrap_ctx);

static void *H5VL_intent_unwrap_object(void *obj);

static herr_t H5VL_intent_free_wrap_ctx(void *obj);

/* Attribute callbacks */
static void *H5VL_intent_attr_create(void *obj,
                                     const H5VL_loc_params_t *loc_params,
                                     const char *name, hid_t type_id,
                                     hid_t space_id, hid_t acpl_id,
                                     hid_t aapl_id, hid_t dxpl_id, void **req);

static void *H5VL_intent_attr_open(void *obj,
                                   const H5VL_loc_params_t *loc_params,
                                   const char *name, hid_t aapl_id,
                                   hid_t dxpl_id, void **req);

static herr_t H5VL_intent_attr_read(void *attr, hid_t mem_type_id, void *buf,
                                    hid_t dxpl_id, void **req);

static herr_t H5VL_intent_attr_write(void *attr, hid_t mem_type_id,
                                     const void *buf, hid_t dxpl_id,
                                     void **req);

static herr_t H5VL_intent_attr_get(void *obj, H5VL_attr_get_args_t *args,
                                   hid_t dxpl_id, void **req);

static herr_t H5VL_intent_attr_specific(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        H5VL_attr_specific_args_t *args,
                                        hid_t dxpl_id, void **req);

static herr_t H5VL_intent_attr_optional(void *obj, H5VL_optional_args_t *args,
                                        hid_t dxpl_id, void **req);

static herr_t H5VL_intent_attr_close(void *attr, hid_t dxpl_id, void **req);

/* Dataset callbacks */
static void *H5VL_intent_dataset_create(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        const char *name, hid_t lcpl_id,
                                        hid_t type_id, hid_t space_id,
                                        hid_t dcpl_id, hid_t dapl_id,
                                        hid_t dxpl_id, void **req);

static void *H5VL_intent_dataset_open(void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      const char *name, hid_t dapl_id,
                                      hid_t dxpl_id, void **req);

static herr_t H5VL_intent_dataset_read(void *dset, hid_t mem_type_id,
                                       hid_t mem_space_id, hid_t file_space_id,
                                       hid_t plist_id, void *buf, void **req);

static herr_t H5VL_intent_dataset_write(void *dset, hid_t mem_type_id,
                                        hid_t mem_space_id, hid_t file_space_id,
                                        hid_t plist_id, const void *buf,
                                        void **req);

static herr_t H5VL_intent_dataset_get(void *dset, H5VL_dataset_get_args_t *args,
                                      hid_t dxpl_id, void **req);

static herr_t H5VL_intent_dataset_specific(void *obj,
                                           H5VL_dataset_specific_args_t *args,
                                           hid_t dxpl_id, void **req);

static herr_t H5VL_intent_dataset_optional(void *obj,
                                           H5VL_optional_args_t *args,
                                           hid_t dxpl_id, void **req);

static herr_t H5VL_intent_dataset_close(void *dset, hid_t dxpl_id, void **req);

/* Datatype callbacks */
static void *H5VL_intent_datatype_commit(void *obj,
                                         const H5VL_loc_params_t *loc_params,
                                         const char *name, hid_t type_id,
                                         hid_t lcpl_id, hid_t tcpl_id,
                                         hid_t tapl_id, hid_t dxpl_id,
                                         void **req);

static void *H5VL_intent_datatype_open(void *obj,
                                       const H5VL_loc_params_t *loc_params,
                                       const char *name, hid_t tapl_id,
                                       hid_t dxpl_id, void **req);

static herr_t H5VL_intent_datatype_get(void *dt, H5VL_datatype_get_args_t *args,
                                       hid_t dxpl_id, void **req);

static herr_t H5VL_intent_datatype_specific(void *obj,
                                            H5VL_datatype_specific_args_t *args,
                                            hid_t dxpl_id, void **req);

static herr_t H5VL_intent_datatype_optional(void *obj,
                                            H5VL_optional_args_t *args,
                                            hid_t dxpl_id, void **req);

static herr_t H5VL_intent_datatype_close(void *dt, hid_t dxpl_id, void **req);

/* File callbacks */
static void *H5VL_intent_file_create(const char *name, unsigned flags,
                                     hid_t fcpl_id, hid_t fapl_id,
                                     hid_t dxpl_id, void **req);

static void *H5VL_intent_file_open(const char *name, unsigned flags,
                                   hid_t fapl_id, hid_t dxpl_id, void **req);

static herr_t H5VL_intent_file_get(void *file, H5VL_file_get_args_t *args,
                                   hid_t dxpl_id, void **req);

static herr_t H5VL_intent_file_specific(void *file,
                                        H5VL_file_specific_args_t *args,
                                        hid_t dxpl_id, void **req);

static herr_t H5VL_intent_file_optional(void *file, H5VL_optional_args_t *args,
                                        hid_t dxpl_id, void **req);

static herr_t H5VL_intent_file_close(void *file, hid_t dxpl_id, void **req);

/* Group callbacks */
static void *H5VL_intent_group_create(void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      const char *name, hid_t lcpl_id,
                                      hid_t gcpl_id, hid_t gapl_id,
                                      hid_t dxpl_id, void **req);

static void *H5VL_intent_group_open(void *obj,
                                    const H5VL_loc_params_t *loc_params,
                                    const char *name, hid_t gapl_id,
                                    hid_t dxpl_id, void **req);

static herr_t H5VL_intent_group_get(void *obj, H5VL_group_get_args_t *args,
                                    hid_t dxpl_id, void **req);

static herr_t H5VL_intent_group_specific(void *obj,
                                         H5VL_group_specific_args_t *args,
                                         hid_t dxpl_id, void **req);

static herr_t H5VL_intent_group_optional(void *obj, H5VL_optional_args_t *args,
                                         hid_t dxpl_id, void **req);

static herr_t H5VL_intent_group_close(void *grp, hid_t dxpl_id, void **req);

/* Link callbacks */
static herr_t H5VL_intent_link_create(H5VL_link_create_args_t *args, void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      hid_t lcpl_id, hid_t lapl_id,
                                      hid_t dxpl_id, void **req);

static herr_t H5VL_intent_link_copy(void *src_obj,
                                    const H5VL_loc_params_t *loc_params1,
                                    void *dst_obj,
                                    const H5VL_loc_params_t *loc_params2,
                                    hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id,
                                    void **req);

static herr_t H5VL_intent_link_move(void *src_obj,
                                    const H5VL_loc_params_t *loc_params1,
                                    void *dst_obj,
                                    const H5VL_loc_params_t *loc_params2,
                                    hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id,
                                    void **req);

static herr_t H5VL_intent_link_get(void *obj,
                                   const H5VL_loc_params_t *loc_params,
                                   H5VL_link_get_args_t *args, hid_t dxpl_id,
                                   void **req);

static herr_t H5VL_intent_link_specific(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        H5VL_link_specific_args_t *args,
                                        hid_t dxpl_id, void **req);

static herr_t H5VL_intent_link_optional(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        H5VL_optional_args_t *args,
                                        hid_t dxpl_id, void **req);

/* Object callbacks */
static void *H5VL_intent_object_open(void *obj,
                                     const H5VL_loc_params_t *loc_params,
                                     H5I_type_t *opened_type, hid_t dxpl_id,
                                     void **req);

static herr_t H5VL_intent_object_copy(void *src_obj,
                                      const H5VL_loc_params_t *src_loc_params,
                                      const char *src_name, void *dst_obj,
                                      const H5VL_loc_params_t *dst_loc_params,
                                      const char *dst_name, hid_t ocpypl_id,
                                      hid_t lcpl_id, hid_t dxpl_id, void **req);

static herr_t H5VL_intent_object_get(void *obj,
                                     const H5VL_loc_params_t *loc_params,
                                     H5VL_object_get_args_t *args,
                                     hid_t dxpl_id, void **req);

static herr_t H5VL_intent_object_specific(void *obj,
                                          const H5VL_loc_params_t *loc_params,
                                          H5VL_object_specific_args_t *args,
                                          hid_t dxpl_id, void **req);

static herr_t H5VL_intent_object_optional(void *obj,
                                          const H5VL_loc_params_t *loc_params,
                                          H5VL_optional_args_t *args,
                                          hid_t dxpl_id, void **req);

/* Container/connector introspection callbacks */
static herr_t H5VL_intent_introspect_get_conn_cls(
    void *obj, H5VL_get_conn_lvl_t lvl, const H5VL_class_t **conn_cls);
#if H5_VERSION_GE(1, 13, 3)
static herr_t H5VL_intent_introspect_get_cap_flags(const void *info,
                                                   uint64_t *cap_flags);
#else
static herr_t H5VL_intent_introspect_get_cap_flags(const void *info,
                                                   unsigned *cap_flags);
#endif
static herr_t H5VL_intent_introspect_opt_query(void *obj, H5VL_subclass_t cls,
                                               int opt_type, uint64_t *flags);

/* Async request callbacks */
static herr_t H5VL_intent_request_wait(void *req, uint64_t timeout,
                                       H5VL_request_status_t *status);

static herr_t H5VL_intent_request_notify(void *obj, H5VL_request_notify_t cb,
                                         void *ctx);

static herr_t H5VL_intent_request_cancel(void *req,
                                         H5VL_request_status_t *status);

static herr_t H5VL_intent_request_specific(void *req,
                                           H5VL_request_specific_args_t *args);

static herr_t H5VL_intent_request_optional(void *req,
                                           H5VL_optional_args_t *args);

static herr_t H5VL_intent_request_free(void *req);

/* Blob callbacks */
static herr_t H5VL_intent_blob_put(void *obj, const void *buf, size_t size,
                                   void *blob_id, void *ctx);

static herr_t H5VL_intent_blob_get(void *obj, const void *blob_id, void *buf,
                                   size_t size, void *ctx);

static herr_t H5VL_intent_blob_specific(void *obj, void *blob_id,
                                        H5VL_blob_specific_args_t *args);

static herr_t H5VL_intent_blob_optional(void *obj, void *blob_id,
                                        H5VL_optional_args_t *args);

/* Token callbacks */
static herr_t H5VL_intent_token_cmp(void *obj, const H5O_token_t *token1,
                                    const H5O_token_t *token2, int *cmp_value);

static herr_t H5VL_intent_token_to_str(void *obj, H5I_type_t obj_type,
                                       const H5O_token_t *token,
                                       char **token_str);

static herr_t H5VL_intent_token_from_str(void *obj, H5I_type_t obj_type,
                                         const char *token_str,
                                         H5O_token_t *token);

/* Generic optional callback */
static herr_t H5VL_intent_optional(void *obj, H5VL_optional_args_t *args,
                                   hid_t dxpl_id, void **req);

/*******************/
/* Local variables */
/*******************/

/* Intent VOL connector class struct */
static const H5VL_class_t H5VL_intent_g = {
    H5VL_VERSION,                          /* VOL class struct version */
    (H5VL_class_value_t)H5VL_INTENT_VALUE, /* value        */
    H5VL_INTENT_NAME,                      /* name         */
    H5VL_INTENT_VERSION,                   /* connector version */
    0,                                     /* capability flags */
    H5VL_intent_init,                      /* initialize   */
    H5VL_intent_term,                      /* terminate    */
    {
        /* info_cls */
        sizeof(H5VL_intent_info_t), /* size    */
        H5VL_intent_info_copy,      /* copy    */
        H5VL_intent_info_cmp,       /* compare */
        H5VL_intent_info_free,      /* free    */
        H5VL_intent_info_to_str,    /* to_str  */
        H5VL_intent_str_to_info     /* from_str */
    },
    {
        /* wrap_cls */
        H5VL_intent_get_object,    /* get_object   */
        H5VL_intent_get_wrap_ctx,  /* get_wrap_ctx */
        H5VL_intent_wrap_object,   /* wrap_object  */
        H5VL_intent_unwrap_object, /* unwrap_object */
        H5VL_intent_free_wrap_ctx  /* free_wrap_ctx */
    },
    {
        /* attribute_cls */
        H5VL_intent_attr_create,   /* create */
        H5VL_intent_attr_open,     /* open */
        H5VL_intent_attr_read,     /* read */
        H5VL_intent_attr_write,    /* write */
        H5VL_intent_attr_get,      /* get */
        H5VL_intent_attr_specific, /* specific */
        H5VL_intent_attr_optional, /* optional */
        H5VL_intent_attr_close     /* close */
    },
    {
        /* dataset_cls */
        H5VL_intent_dataset_create,   /* create */
        H5VL_intent_dataset_open,     /* open */
        H5VL_intent_dataset_read,     /* read */
        H5VL_intent_dataset_write,    /* write */
        H5VL_intent_dataset_get,      /* get */
        H5VL_intent_dataset_specific, /* specific */
        H5VL_intent_dataset_optional, /* optional */
        H5VL_intent_dataset_close     /* close */
    },
    {
        /* datatype_cls */
        H5VL_intent_datatype_commit,   /* commit */
        H5VL_intent_datatype_open,     /* open */
        H5VL_intent_datatype_get,      /* get_size */
        H5VL_intent_datatype_specific, /* specific */
        H5VL_intent_datatype_optional, /* optional */
        H5VL_intent_datatype_close     /* close */
    },
    {
        /* file_cls */
        H5VL_intent_file_create,   /* create */
        H5VL_intent_file_open,     /* open */
        H5VL_intent_file_get,      /* get */
        H5VL_intent_file_specific, /* specific */
        H5VL_intent_file_optional, /* optional */
        H5VL_intent_file_close     /* close */
    },
    {
        /* group_cls */
        H5VL_intent_group_create,   /* create */
        H5VL_intent_group_open,     /* open */
        H5VL_intent_group_get,      /* get */
        H5VL_intent_group_specific, /* specific */
        H5VL_intent_group_optional, /* optional */
        H5VL_intent_group_close     /* close */
    },
    {
        /* link_cls */
        H5VL_intent_link_create,   /* create */
        H5VL_intent_link_copy,     /* copy */
        H5VL_intent_link_move,     /* move */
        H5VL_intent_link_get,      /* get */
        H5VL_intent_link_specific, /* specific */
        H5VL_intent_link_optional  /* optional */
    },
    {
        /* object_cls */
        H5VL_intent_object_open,     /* open */
        H5VL_intent_object_copy,     /* copy */
        H5VL_intent_object_get,      /* get */
        H5VL_intent_object_specific, /* specific */
        H5VL_intent_object_optional  /* optional */
    },
    {
        /* introspect_cls */
        H5VL_intent_introspect_get_conn_cls,  /* get_conn_cls */
        H5VL_intent_introspect_get_cap_flags, /* get_cap_flags */
        H5VL_intent_introspect_opt_query,     /* opt_query */
    },
    {
        /* request_cls */
        H5VL_intent_request_wait,     /* wait */
        H5VL_intent_request_notify,   /* notify */
        H5VL_intent_request_cancel,   /* cancel */
        H5VL_intent_request_specific, /* specific */
        H5VL_intent_request_optional, /* optional */
        H5VL_intent_request_free      /* free */
    },
    {
        /* blob_cls */
        H5VL_intent_blob_put,      /* put */
        H5VL_intent_blob_get,      /* get */
        H5VL_intent_blob_specific, /* specific */
        H5VL_intent_blob_optional  /* optional */
    },
    {
        /* token_cls */
        H5VL_intent_token_cmp,     /* cmp */
        H5VL_intent_token_to_str,  /* to_str */
        H5VL_intent_token_from_str /* from_str */
    },
    H5VL_intent_optional /* optional */
};

/* The connector identification number, initialized at runtime */
static hid_t H5VL_INTENT_g = H5I_INVALID_HID;

H5PL_type_t H5PLget_plugin_type(void) { return H5PL_TYPE_VOL; }
const void *H5PLget_plugin_info(void) { return &H5VL_intent_g; }

/*-------------------------------------------------------------------------
 * Function:    H5VL__intent_new_obj
 *
 * Purpose:     Create a new intent object for an underlying object
 *
 * Return:      Success:    Pointer to the new intent object
 *              Failure:    NULL
 *
 * Programmer:  Hariharan Devarajan
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
static H5VL_intent_t *H5VL_intent_new_obj(void *under_obj, hid_t under_vol_id) {
  H5VL_intent_t *new_obj;

  new_obj = (H5VL_intent_t *)calloc(1, sizeof(H5VL_intent_t));
  new_obj->under_object = under_obj;
  new_obj->under_vol_id = under_vol_id;
  H5Iinc_ref(new_obj->under_vol_id);

  return new_obj;
} /* end H5VL__intent_new_obj() */

/*-------------------------------------------------------------------------
 * Function:    H5VL__intent_free_obj
 *
 * Purpose:     Release a intent object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 * Programmer:  Hariharan Devarajan
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_free_obj(H5VL_intent_t *obj) {
  hid_t err_id;
  err_id = H5Eget_current_stack();
  H5Idec_ref(obj->under_vol_id);
  H5Eset_current_stack(err_id);
  free(obj);
  return 0;
} /* end H5VL__intent_free_obj() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_register
 *
 * Purpose:     Register the intent VOL connector and retrieve an ID
 *              for it.
 *
 * Return:      Success:    The ID for the intent VOL connector
 *              Failure:    -1
 *
 * Programmer:  Hariharan Devarajan
 *              Wednesday, November 28, 2018
 *
 *-------------------------------------------------------------------------
 */
hid_t H5VL_intent_register(void) {
  /* Singleton register the intent VOL connector ID */
  if (H5VL_INTENT_g < 0)
    H5VL_INTENT_g = H5VLregister_connector(&H5VL_intent_g, H5P_DEFAULT);
  return H5VL_INTENT_g;
} /* end H5VL_intent_register() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_init
 *
 * Purpose:     Initialize this VOL connector, performing any necessary
 *              operations for the connector that will apply to all containers
 *              accessed with the connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_init(hid_t vipl_id) {
#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL INIT");
#endif

  /* Shut compiler up about unused parameter */
  (void)vipl_id;

  return 0;
} /* end H5VL_intent_init() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_term
 *
 * Purpose:     Terminate this VOL connector, performing any necessary
 *              operations for the connector that release connector-wide
 *              resources (usually created / initialized with the 'init'
 *              callback).
 *
 * Return:      Success:    0
 *              Failure:    (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_term(void) {
#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL TERM");
#endif

  /* Reset VOL ID */
  H5VL_INTENT_g = H5I_INVALID_HID;

  return 0;
} /* end H5VL_intent_term() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_info_copy
 *
 * Purpose:     Duplicate the connector's info object.
 *
 * Returns:     Success:    New connector info object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
static void *H5VL_intent_info_copy(const void *_info) {
  const H5VL_intent_info_t *info = (const H5VL_intent_info_t *)_info;
  H5VL_intent_info_t *new_info;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL INFO Copy");
#endif

  /* Allocate new VOL info struct for the intent connector */
  new_info = (H5VL_intent_info_t *)calloc(1, sizeof(H5VL_intent_info_t));

  /* Increment reference count on underlying VOL ID, and copy the VOL info */
  new_info->under_vol_id = info->under_vol_id;
  H5Iinc_ref(new_info->under_vol_id);
  if (info->under_vol_info)
    H5VLcopy_connector_info(new_info->under_vol_id, &(new_info->under_vol_info),
                            info->under_vol_info);

  return new_info;
} /* end H5VL_intent_info_copy() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_info_cmp
 *
 * Purpose:     Compare two of the connector's info objects, setting *cmp_value,
 *              following the same rules as strcmp().
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_info_cmp(int *cmp_value, const void *_info1,
                                   const void *_info2) {
  const H5VL_intent_info_t *info1 = (const H5VL_intent_info_t *)_info1;
  const H5VL_intent_info_t *info2 = (const H5VL_intent_info_t *)_info2;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL INFO Compare");
#endif

  /* Sanity checks */
  assert(info1);
  assert(info2);

  /* Initialize comparison value */
  *cmp_value = 0;

  /* Compare under VOL connector classes */
  H5VLcmp_connector_cls(cmp_value, info1->under_vol_id, info2->under_vol_id);
  if (*cmp_value != 0) return 0;

  /* Compare under VOL connector info objects */
  H5VLcmp_connector_info(cmp_value, info1->under_vol_id, info1->under_vol_info,
                         info2->under_vol_info);
  if (*cmp_value != 0) return 0;

  return 0;
} /* end H5VL_intent_info_cmp() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_info_free
 *
 * Purpose:     Release an info object for the connector.
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_info_free(void *_info) {
  H5VL_intent_info_t *info = (H5VL_intent_info_t *)_info;
  hid_t err_id;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL INFO Free");
#endif

  err_id = H5Eget_current_stack();

  /* Release underlying VOL ID and info */
  if (info->under_vol_info)
    H5VLfree_connector_info(info->under_vol_id, info->under_vol_info);
  H5Idec_ref(info->under_vol_id);

  H5Eset_current_stack(err_id);

  /* Free intent info object itself */
  free(info);

  return 0;
} /* end H5VL_intent_info_free() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_info_to_str
 *
 * Purpose:     Serialize an info object for this connector into a string
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_info_to_str(const void *_info, char **str) {
  const H5VL_intent_info_t *info = (const H5VL_intent_info_t *)_info;
  H5VL_class_value_t under_value = (H5VL_class_value_t)-1;
  char *under_vol_string = NULL;
  size_t under_vol_str_len = 0;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL INFO To String");
#endif

  /* Get value and string for underlying VOL connector */
  H5VLget_value(info->under_vol_id, &under_value);
  H5VLconnector_info_to_str(info->under_vol_info, info->under_vol_id,
                            &under_vol_string);

  /* Determine length of underlying VOL info string */
  if (under_vol_string) under_vol_str_len = strlen(under_vol_string);

  /* Allocate space for our info */
  *str = (char *)H5allocate_memory(32 + under_vol_str_len, (hbool_t)0);
  assert(*str);

  /* Encode our info
   * Normally we'd use snprintf() here for a little extra safety, but that
   * call had problems on Windows until recently. So, to be as
   * platform-independent as we can, we're using sprintf() instead.
   */
  sprintf(*str, "under_vol=%u;under_info={%s}", (unsigned)under_value,
          (under_vol_string ? under_vol_string : ""));

  return 0;
} /* end H5VL_intent_info_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_str_to_info
 *
 * Purpose:     Deserialize a string into an info object for this connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_str_to_info(const char *str, void **_info) {
  H5VL_intent_info_t *info;
  unsigned under_vol_value;
  const char *under_vol_info_start, *under_vol_info_end;
  hid_t under_vol_id;
  void *under_vol_info = NULL;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL INFO String To Info");
#endif

  /* Retrieve the underlying VOL connector value and info */
  sscanf(str, "under_vol=%u;", &under_vol_value);
  under_vol_id = H5VLregister_connector_by_value(
      (H5VL_class_value_t)under_vol_value, H5P_DEFAULT);
  under_vol_info_start = strchr(str, '{');
  under_vol_info_end = strrchr(str, '}');
  assert(under_vol_info_end > under_vol_info_start);
  if (under_vol_info_end != (under_vol_info_start + 1)) {
    char *under_vol_info_str;

    under_vol_info_str =
        (char *)malloc((size_t)(under_vol_info_end - under_vol_info_start));
    memcpy(under_vol_info_str, under_vol_info_start + 1,
           (size_t)((under_vol_info_end - under_vol_info_start) - 1));
    *(under_vol_info_str + (under_vol_info_end - under_vol_info_start - 1)) = '\0';

    H5VLconnector_str_to_info(under_vol_info_str, under_vol_id,
                              &under_vol_info);
    //sleep(30);
    char* conf;
    bool is_selected = select_correct_conf(under_vol_info_str, &conf);
    if (is_selected) {
      load_configuration(conf);
      cpp_logger_clog_level(CPP_LOGGER_ERROR, H5_INTENT_LOG_NAME);
    }
    free(under_vol_info_str);
  } /* end else */

  /* Allocate new intent VOL connector info and set its fields */
  info = (H5VL_intent_info_t *)calloc(1, sizeof(H5VL_intent_info_t));
  info->under_vol_id = under_vol_id;
  info->under_vol_info = under_vol_info;

  /* Set return value */
  *_info = info;

  return 0;
} /* end H5VL_intent_str_to_info() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_get_object
 *
 * Purpose:     Retrieve the 'data' for a VOL object.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static void *H5VL_intent_get_object(const void *obj) {
  const H5VL_intent_t *o = (const H5VL_intent_t *)obj;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL Get object");
#endif

  return H5VLget_object(o->under_object, o->under_vol_id);
} /* end H5VL_intent_get_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_get_wrap_ctx
 *
 * Purpose:     Retrieve a "wrapper context" for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_get_wrap_ctx(const void *obj, void **wrap_ctx) {
  const H5VL_intent_t *o = (const H5VL_intent_t *)obj;
  H5VL_intent_wrap_ctx_t *new_wrap_ctx;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL WRAP CTX Get");
#endif

  /* Allocate new VOL object wrapping context for the intent connector */
  new_wrap_ctx =
      (H5VL_intent_wrap_ctx_t *)calloc(1, sizeof(H5VL_intent_wrap_ctx_t));

  /* Increment reference count on underlying VOL ID, and copy the VOL info */
  new_wrap_ctx->under_vol_id = o->under_vol_id;
  H5Iinc_ref(new_wrap_ctx->under_vol_id);
  H5VLget_wrap_ctx(o->under_object, o->under_vol_id,
                   &new_wrap_ctx->under_wrap_ctx);

  /* Set wrap context to return */
  *wrap_ctx = new_wrap_ctx;

  return 0;
} /* end H5VL_intent_get_wrap_ctx() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_wrap_object
 *
 * Purpose:     Use a "wrapper context" to wrap a data object
 *
 * Return:      Success:    Pointer to wrapped object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
static void *H5VL_intent_wrap_object(void *obj, H5I_type_t obj_type,
                                     void *_wrap_ctx) {
  H5VL_intent_wrap_ctx_t *wrap_ctx = (H5VL_intent_wrap_ctx_t *)_wrap_ctx;
  H5VL_intent_t *new_obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL WRAP Object");
#endif

  /* Wrap the object with the underlying VOL */
  under = H5VLwrap_object(obj, obj_type, wrap_ctx->under_vol_id,
                          wrap_ctx->under_wrap_ctx);
  if (under)
    new_obj = H5VL_intent_new_obj(under, wrap_ctx->under_vol_id);
  else
    new_obj = NULL;

  return new_obj;
} /* end H5VL_intent_wrap_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_unwrap_object
 *
 * Purpose:     Unwrap a wrapped object, discarding the wrapper, but returning
 *		underlying object.
 *
 * Return:      Success:    Pointer to unwrapped object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
static void *H5VL_intent_unwrap_object(void *obj) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL UNWRAP Object");
#endif

  /* Unrap the object with the underlying VOL */
  under = H5VLunwrap_object(o->under_object, o->under_vol_id);

  if (under) H5VL_intent_free_obj(o);

  return under;
} /* end H5VL_intent_unwrap_object() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_free_wrap_ctx
 *
 * Purpose:     Release a "wrapper context" for an object
 *
 * Note:	Take care to preserve the current HDF5 error stack
 *		when calling HDF5 API calls.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_free_wrap_ctx(void *_wrap_ctx) {
  H5VL_intent_wrap_ctx_t *wrap_ctx = (H5VL_intent_wrap_ctx_t *)_wrap_ctx;
  hid_t err_id;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL WRAP CTX Free");
#endif

  err_id = H5Eget_current_stack();

  /* Release underlying VOL ID and wrap context */
  if (wrap_ctx->under_wrap_ctx)
    H5VLfree_wrap_ctx(wrap_ctx->under_wrap_ctx, wrap_ctx->under_vol_id);
  H5Idec_ref(wrap_ctx->under_vol_id);

  H5Eset_current_stack(err_id);

  /* Free intent wrap context object itself */
  free(wrap_ctx);

  return 0;
} /* end H5VL_intent_free_wrap_ctx() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_attr_create
 *
 * Purpose:     Creates an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_attr_create(void *obj,
                                     const H5VL_loc_params_t *loc_params,
                                     const char *name, hid_t type_id,
                                     hid_t space_id, hid_t acpl_id,
                                     hid_t aapl_id, hid_t dxpl_id, void **req) {
  H5VL_intent_t *attr;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL ATTRIBUTE Create");
#endif

  under = H5VLattr_create(o->under_object, loc_params, o->under_vol_id, name,
                          type_id, space_id, acpl_id, aapl_id, dxpl_id, req);
  if (under) {
    attr = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    attr = NULL;

  return (void *)attr;
} /* end H5VL_intent_attr_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_attr_open
 *
 * Purpose:     Opens an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_attr_open(void *obj,
                                   const H5VL_loc_params_t *loc_params,
                                   const char *name, hid_t aapl_id,
                                   hid_t dxpl_id, void **req) {
  H5VL_intent_t *attr;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL ATTRIBUTE Open");
#endif

  under = H5VLattr_open(o->under_object, loc_params, o->under_vol_id, name,
                        aapl_id, dxpl_id, req);
  if (under) {
    attr = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    attr = NULL;

  return (void *)attr;
} /* end H5VL_intent_attr_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_attr_read
 *
 * Purpose:     Reads data from attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_attr_read(void *attr, hid_t mem_type_id, void *buf,
                                    hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)attr;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL ATTRIBUTE Read");
#endif

  ret_value = H5VLattr_read(o->under_object, o->under_vol_id, mem_type_id, buf,
                            dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_attr_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_attr_write
 *
 * Purpose:     Writes data to attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_attr_write(void *attr, hid_t mem_type_id,
                                     const void *buf, hid_t dxpl_id,
                                     void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)attr;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL ATTRIBUTE Write");
#endif

  ret_value = H5VLattr_write(o->under_object, o->under_vol_id, mem_type_id, buf,
                             dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_attr_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_attr_get
 *
 * Purpose:     Gets information about an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_attr_get(void *obj, H5VL_attr_get_args_t *args,
                                   hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL ATTRIBUTE Get");
#endif

  ret_value =
      H5VLattr_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_attr_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_attr_specific
 *
 * Purpose:     Specific operation on attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_attr_specific(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        H5VL_attr_specific_args_t *args,
                                        hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL ATTRIBUTE Specific");
#endif

  ret_value = H5VLattr_specific(o->under_object, loc_params, o->under_vol_id,
                                args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_attr_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_attr_optional
 *
 * Purpose:     Perform a connector-specific operation on an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_attr_optional(void *obj, H5VL_optional_args_t *args,
                                        hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL ATTRIBUTE Optional");
#endif

  ret_value =
      H5VLattr_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_attr_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_attr_close
 *
 * Purpose:     Closes an attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1, attr not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_attr_close(void *attr, hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)attr;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL ATTRIBUTE Close");
#endif

  ret_value = H5VLattr_close(o->under_object, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying attribute was closed */
  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_attr_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_dataset_create
 *
 * Purpose:     Creates a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_dataset_create(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        const char *name, hid_t lcpl_id,
                                        hid_t type_id, hid_t space_id,
                                        hid_t dcpl_id, hid_t dapl_id,
                                        hid_t dxpl_id, void **req) {
  H5VL_intent_t *dset;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- INTENT VOL DATASET Create");
#endif
  char name_fqn[256];
  int name_size = 0;
  H5VL_object_get_args_t args;
  args.op_type = H5VL_OBJECT_GET_NAME;
  args.args.get_name.buf = name_fqn;
  args.args.get_name.buf_size = 256;
  args.args.get_name.name_len = &name_size;
  name_size = *args.args.get_name.name_len;

  herr_t ret_value = H5VLobject_get(o->under_object, loc_params,
                                    o->under_vol_id, &args, dxpl_id, req);
  size_t dset_size = strlen(name);
  strcpy(name_fqn, args.args.get_name.buf);
  strcpy(name_fqn + name_size, name);
  name_fqn[name_size + dset_size] = '\0';
  struct DatasetProperties datasetProperties;
  bool is_present = get_dataset_properties(name_fqn, &datasetProperties);
  if (is_present) {
#ifdef ENABLE_INTENT_LOGGING
    H5INTENT_LOGINFO("------- INTENT VOL DATASET Found properties for dataset %s", name_fqn);
#endif
    if (datasetProperties.access.append_flush.use) {
      /**
       * TODO: Probably just a callback
       * When a user is appending data to a dataset via H5DOappend() and the
       * dataset’s newly extended dimension size hits a specified boundary, the
       * library will perform the first action listed above. Upon return from
       * the callback function, the library will then perform the second action
       * listed above and return to the user. If no boundary is hit or set, the
       * two actions above are not invoked. herr_t H5Pset_append_flush(hid_t
       * dapl_id, unsigned ndims, const hsize_t boundary[], H5D_append_cb_t
       * func, void * 	udata)
       *
       * Sets two actions to perform when the size of a dataset’s dimension
       * being appended reaches a specified boundary. Parameters [in] dapl_id
       * Dataset access property list identifier [in]	ndims	The number of
       * elements for boundary
       *    [in]	boundary	The dimension sizes used to determine the
       * boundary [in]	func	The user-defined callback function [in] udata
       * The user-defined input data Returns Returns a non-negative value if
       * successful; otherwise returns a negative value.
       */
    }
    if (datasetProperties.access.chunk.use) {
      /**
       * H5Pset_chunk()
       * herr_t H5Pset_chunk(hid_t plist_id, int ndims, const hsize_t dim[])
       * Sets the size of the chunks used to store a chunked layout dataset.
       * Parameters
       *    [in]	plist_id	Dataset creation property list identifier
       *    [in]	ndims	The number of dimensions of each chunk
       *    [in]	dim	An array defining the size, in dataset elements, of each chunk
       * Returns
       *    Returns a non-negative value if successful; otherwise returns a negative value.
       *
       * H5Pset_chunk() sets the size of the chunks used to store a chunked layout dataset.
       * This function is only valid for dataset creation property lists.
       * The ndims parameter currently must be the same size as the rank of the dataset.
       * The values of the dim array define the size of the chunks to store the dataset's
       * raw data. The unit of measure for dim values is dataset elements.
       * As a side-effect of this function, the layout of the dataset is changed to
       * H5D_CHUNKED, if it is not already so set.
       */
      herr_t status = H5Pset_chunk(dcpl_id,
                                   datasetProperties.access.chunk.ndims,
                                   datasetProperties.access.chunk.dim);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting chunk for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting chunk for dataset %s successful", name_fqn);
      }
      if (datasetProperties.access.chunk.opts > 0) {
        status = H5Pset_chunk_opts(dcpl_id, datasetProperties.access.chunk.opts);
        if (status != 0) {
          H5INTENT_LOGERROR("DATASET setting chunk opts for dataset %s failed", name_fqn);
        }else {
          H5INTENT_LOGINFO("DATASET setting chunk opts for dataset %s successful", name_fqn);
        }
      }
    }
    if (datasetProperties.access.chunk_cache.use) {
      /**
       * H5Pset_chunk_cache()
       * herr_t H5Pset_chunk_cache (hid_t dapl_id, size_t rdcc_nslots,
       *                                size_t rdcc_nbytes, double rdcc_w0)
       * Sets the raw data chunk cache parameters.
       * Parameters
       *    [in]	dapl_id	Dataset access property list identifier
       *    [in]	rdcc_nslots	The number of chunk slots in the raw data
       *                chunk cache for this dataset. Increasing this value reduces
       *                the number of cache collisions, but slightly increases the
       *                memory used. Due to the hashing strategy, this value should
       *                ideally be a prime number. As a rule of thumb, this value
       *                should be at least 10 times the number of chunks that can
       *                fit in rdcc_nbytes bytes. For maximum performance, this
       *                value should be set approximately 100 times that number
       *                of chunks. The default value is 521. If the value passed
       *                is H5D_CHUNK_CACHE_NSLOTS_DEFAULT, then the property will
       *                not be set on dapl_id and the parameter will come from the
       *                file access property list used to open the file.
       *    [in]	rdcc_nbytes	The total size of the raw data chunk cache
       *                for this dataset. In most cases increasing this number will
       *                improve performance, as long as you have enough free memory.
       *                The default size is 1 MB. If the value passed is
       *                H5D_CHUNK_CACHE_NBYTES_DEFAULT, then the property will not
       *                be set on dapl_id and the parameter will come from the file
       *                access property list.
       *   [in]	        rdcc_w0	The chunk preemption policy for this dataset.
       *                This must be between 0 and 1 inclusive and indicates the
       *                weighting according to which chunks which have been fully
       *                read or written are penalized when determining which chunks
       *                to flush from cache. A value of 0 means fully read or written
       *                chunks are treated no differently than other chunks (the
       *                preemption is strictly LRU) while a value of 1 means fully
       *                read or written chunks are always preempted before other chunks.
       *                If your application only reads or writes data once, this
       *                can be safely set to 1. Otherwise, this should be set lower,
       *                depending on how often you re-read or re-write the same data.
       *                The default value is 0.75. If the value passed is
       *                H5D_CHUNK_CACHE_W0_DEFAULT, then the property will not
       *                be set on dapl_id and the parameter will come from the
       *                file access property list.
       * Returns
       *    Returns a non-negative value if successful; otherwise returns a negative value.
       *
       * H5Pset_chunk_cache() sets the number of elements, the total number of bytes,
       * and the preemption policy value in the raw data chunk cache on a dataset
       * access property list. After calling this function, the values set in the
       * property list will override the values in the file's file access property
       * list. The raw data chunk cache inserts chunks into the cache by first
       * computing a hash value using the address of a chunk, then using that
       * hash value as the chunk's index into the table of cached chunks. The
       * size of this hash table, i.e., and the number of possible hash values,
       * is determined by the rdcc_nslots parameter. If a different chunk in the
       * cache has the same hash value, this causes a collision, which reduces
       * efficiency. If inserting the chunk into cache would cause the cache to
       * be too big, then the cache is pruned according to the rdcc_w0 parameter.
       *
       */
       herr_t status = H5Pset_chunk_cache(dapl_id,
                                         datasetProperties.access.chunk_cache.rdcc_nslots,
                                         datasetProperties.access.chunk_cache.rdcc_nbytes,
                                         datasetProperties.access.chunk_cache.rdcc_w0);
       if (status != 0) {
          H5INTENT_LOGERROR("DATASET setting chunk_cache for dataset %s failed", name_fqn);
       } else {
          H5INTENT_LOGINFO("DATASET setting chunk_cache for dataset %s successful",
                         name_fqn);
       }
    }
    if (datasetProperties.access.filter_avail.use) {
    }
    if (datasetProperties.access.gzip.use) {
    }
    if (datasetProperties.access.layout.use) {
      herr_t status = H5Pset_layout(dcpl_id, datasetProperties.access.layout.layout);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting layout for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting layout for dataset %s successful", name_fqn);
      }
    }
    if (datasetProperties.access.szip.use) {
    }
    if (datasetProperties.access.virtual_view.use) {
    }
    if (datasetProperties.transfer.buffer.use) {
    }
    if (datasetProperties.transfer.hyper_vector.use) {
      /**
       * H5Pset_hyper_vector_size()
       * herr_t H5Pset_hyper_vector_size(hid_t 	fapl_id, size_t size)
       * Sets number of I/O vectors to be read/written in hyperslab I/O.
       * Parameters
       *    [in]	fapl_id	Dataset transfer property list identifier
       *    [in]	size	Number of I/O vectors to accumulate in memory
       *    for I/O operations
       * Returns
       *    Returns a non-negative value if successful; otherwise
       *    returns a negative value.
       *
       *    H5Pset_hyper_vector_size() sets the number of I/O vectors to be
       *    accumulated in memory before being issued to the lower levels of the
       *    HDF5 library for reading or writing the actual data.
       *
       *    The I/O vectors are hyperslab offset and length pairs and are
       *    generated during hyperslab I/O. The number of I/O vectors is
       *    passed in size to be set in the dataset transfer property list
       *    plist_id. size must be greater than 1 (one).
       *
       *    H5Pset_hyper_vector_size() is an I/O optimization function;
       *    increasing vector_size should provide better performance, but
       *    the library will use more memory during hyperslab I/O.
       *    The default value of size is 1024.]
       **/
      hsize_t total_size = 1;
      for(int index = 0; index<datasetProperties.transfer.hyper_vector.ndims;++index)
        total_size *= datasetProperties.transfer.hyper_vector.size[index];
      herr_t status = H5Pset_hyper_vector_size(dxpl_id, total_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting hyper_vector_size for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting hyper_vector_size for dataset %s successful", name_fqn);
      }
    }
    if (datasetProperties.transfer.dataset_io_hyperslab_selection.use) {
      /**
       * H5Pset_dataset_io_hyperslab_selection()
       * Sets a hyperslab file selection for a dataset I/O operation.
       * Parameters
       *    [in]	plist_id	Property list identifier
       *    [in]	rank	Number of dimensions of selection
       *    [in]	op	Operation to perform on current selection
       *    [in]	start	Offset of start of hyperslab
       *    [in]	stride	Hyperslab stride
       *    [in]	count	Number of blocks included in hyperslab
       *    [in]	block	Size of block in hyperslab
       *    Returns     Returns a non-negative value if successful;
       *                otherwise returns a negative value.
       *
       * H5Pset_dataset_io_hyperslab_selection() is designed to be used in
       * conjunction with using H5S_PLIST for the file dataspace ID when
       * making a call to H5Dread() or H5Dwrite(). When used with H5S_PLIST,
       * the selection created by one or more calls to this routine is used
       * for determining which dataset elements to access. rank is the
       * dimensionality of the selection and determines the size of the start,
       * stride, count, and block arrays. rank must be between 1 and
       * H5S_MAX_RANK, inclusive. The op, start, stride, count, and block
       * parameters behave identically to their behavior for
       * H5Sselect_hyperslab(), please see the documentation for that
       * routine for details about their use.
       */
      herr_t status = H5Pset_dataset_io_hyperslab_selection(dxpl_id,
                                            datasetProperties.transfer.dataset_io_hyperslab_selection.rank,
                                            datasetProperties.transfer.dataset_io_hyperslab_selection.op,
                                            datasetProperties.transfer.dataset_io_hyperslab_selection.start,
                                            datasetProperties.transfer.dataset_io_hyperslab_selection.stride,
                                            datasetProperties.transfer.dataset_io_hyperslab_selection.count,
                                            datasetProperties.transfer.dataset_io_hyperslab_selection.block);
      if (status > 0) {
        H5INTENT_LOGERROR("DATASET setting dataset_io_hyperslab_selection for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting dataset_io_hyperslab_selection for dataset %s successful", name_fqn);
      }
    }
    if (datasetProperties.transfer.dmpiio.use) {
      herr_t status = H5Pset_dxpl_mpio(dxpl_id, datasetProperties.transfer.dmpiio.xfer_mode);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting H5Pset_dxpl_mpio for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting H5Pset_dxpl_mpio for dataset %s successful", name_fqn);
      }
      status = H5Pset_dxpl_mpio_collective_opt(dxpl_id, datasetProperties.transfer.dmpiio.coll_opt_mode);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting H5Pset_dxpl_mpio_collective_opt for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting H5Pset_dxpl_mpio_collective_opt for dataset %s successful", name_fqn);
      }
      status = H5Pset_dxpl_mpio_chunk_opt_num(dxpl_id, datasetProperties.transfer.dmpiio.num_chunk_per_proc);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting H5Pset_dxpl_mpio_chunk_opt_num for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting H5Pset_dxpl_mpio_chunk_opt_num for dataset %s successful", name_fqn);
      }
      status = H5Pset_dxpl_mpio_chunk_opt_ratio(dxpl_id, datasetProperties.transfer.dmpiio.percent_num_proc_per_chunk);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting H5Pset_dxpl_mpio_chunk_opt_ratio for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting H5Pset_dxpl_mpio_chunk_opt_ratio for dataset %s successful", name_fqn);
      }
      status = H5Pset_dxpl_mpio_chunk_opt(dxpl_id, datasetProperties.transfer.dmpiio.chunk_opt_mode);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting H5Pset_dxpl_mpio_chunk_opt for dataset %s failed", name_fqn);
      } else {
        H5INTENT_LOGINFO("DATASET setting H5Pset_dxpl_mpio_chunk_opt for dataset %s successful", name_fqn);
      }
    }
    if (datasetProperties.transfer.edc_check.use) {
    }
    if (datasetProperties.transfer.mem_manager.use) {
    }
  } else {
#ifdef ENABLE_INTENT_LOGGING
    H5INTENT_LOGINFO(
        "DATASET Did not find properties for dataset %s",
        name);
#endif
  }
  under = H5VLdataset_create(o->under_object, loc_params, o->under_vol_id, name,
                             lcpl_id, type_id, space_id, dcpl_id, dapl_id,
                             dxpl_id, req);
  if (under) {
    dset = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    dset = NULL;

  return (void *)dset;
} /* end H5VL_intent_dataset_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_dataset_open
 *
 * Purpose:     Opens a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_dataset_open(void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      const char *name, hid_t dapl_id,
                                      hid_t dxpl_id, void **req) {
  H5VL_intent_t *dset;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATASET Open");
#endif
  char name_fqn[256];
  int name_size = 0;
  H5VL_object_get_args_t args;
  args.op_type = H5VL_OBJECT_GET_NAME;
  args.args.get_name.buf = name_fqn;
  args.args.get_name.buf_size = 256;
  args.args.get_name.name_len = &name_size;
  name_size = *args.args.get_name.name_len;

  herr_t ret_value = H5VLobject_get(o->under_object, loc_params,
                                    o->under_vol_id, &args, dxpl_id, req);
  size_t dset_size = strlen(name);
  strcpy(name_fqn, args.args.get_name.buf);
  strcpy(name_fqn + name_size, name);
  name_fqn[name_size + dset_size] = '\0';
  struct DatasetProperties datasetProperties;
  bool is_present = get_dataset_properties(name_fqn, &datasetProperties);
  if (is_present) {
#ifdef ENABLE_INTENT_LOGGING
    H5INTENT_LOGINFO(
        "------- INTENT VOL DATASET Found properties for dataset %s", name_fqn);
#endif
    if (datasetProperties.access.append_flush.use) {
      /**
       * TODO: Probably just a callback
       * When a user is appending data to a dataset via H5DOappend() and the
       * dataset’s newly extended dimension size hits a specified boundary, the
       * library will perform the first action listed above. Upon return from
       * the callback function, the library will then perform the second action
       * listed above and return to the user. If no boundary is hit or set, the
       * two actions above are not invoked. herr_t H5Pset_append_flush(hid_t
       * dapl_id, unsigned ndims, const hsize_t boundary[], H5D_append_cb_t
       * func, void * 	udata)
       *
       * Sets two actions to perform when the size of a dataset’s dimension
       * being appended reaches a specified boundary. Parameters [in] dapl_id
       * Dataset access property list identifier [in]	ndims	The number of
       * elements for boundary
       *    [in]	boundary	The dimension sizes used to determine the boundary [in]	func	The user-defined callback function [in] udata The user-defined input data Returns Returns a non-negative value if successful; otherwise returns a negative value.
       */
    }
    if (datasetProperties.access.chunk_cache.use) {
      /**
       * H5Pset_chunk_cache()
       * herr_t H5Pset_chunk_cache (hid_t dapl_id, size_t rdcc_nslots,
       *                                size_t rdcc_nbytes, double rdcc_w0)
       * Sets the raw data chunk cache parameters.
       * Parameters
       *    [in]	dapl_id	Dataset access property list identifier
       *    [in]	rdcc_nslots	The number of chunk slots in the raw data chunk cache for this dataset. Increasing this value reduces the number of cache collisions, but slightly increases the memory used. Due to the hashing strategy, this value should ideally be a prime number. As a rule of thumb, this value should be at least 10 times the number of chunks that can fit in rdcc_nbytes bytes. For maximum performance, this value should be set approximately 100 times that number of chunks. The default value is 521. If the value passed is H5D_CHUNK_CACHE_NSLOTS_DEFAULT, then the property will not be set on dapl_id and the parameter will come from the file access property list used to open the file. [in]	rdcc_nbytes	The total size of the raw data chunk cache for this dataset. In most cases increasing this number will improve performance, as long as you have enough free memory. The default size is 1 MB. If the value passed is
       *                H5D_CHUNK_CACHE_NBYTES_DEFAULT, then the property will not be set on dapl_id and the parameter will come from the file access property list. [in]	        rdcc_w0	The chunk preemption policy for this dataset. This must be between 0 and 1 inclusive and indicates the
       *                weighting according to which chunks which have been fully read or written are penalized when determining which chunks to flush from cache. A value of 0 means fully read or written chunks are treated no differently than other chunks (the preemption is strictly LRU) while a value of 1 means fully read or written chunks are always preempted before other chunks. If your application only reads or writes data once, this can be safely set to 1. Otherwise, this should be set lower, depending on how often you re-read or re-write the same data.
       *                The default value is 0.75. If the value passed is
       *                H5D_CHUNK_CACHE_W0_DEFAULT, then the property will not
       *                be set on dapl_id and the parameter will come from the
       *                file access property list.
       * Returns
       *    Returns a non-negative value if successful; otherwise returns a negative value.
       *
       * H5Pset_chunk_cache() sets the number of elements, the total number of bytes, and the preemption policy value in the raw data chunk cache on a dataset access property list. After calling this function, the values set in the property list will override the values in the file's file access property list. The raw data chunk cache inserts chunks into the cache by first computing a hash value using the address of a chunk, then using that hash value as the chunk's index into the table of cached chunks. The size of this hash table, i.e., and the number of possible hash values, is determined by the rdcc_nslots parameter. If a different chunk in the cache has the same hash value, this causes a collision, which reduces efficiency. If inserting the chunk into cache would cause the cache to be too big, then the cache is pruned according to the rdcc_w0 parameter.
       *
       */
      herr_t status = H5Pset_chunk_cache(
          dapl_id, datasetProperties.access.chunk_cache.rdcc_nslots,
          datasetProperties.access.chunk_cache.rdcc_nbytes,
          datasetProperties.access.chunk_cache.rdcc_w0);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting chunk_cache for dataset %s failed",
                          name_fqn);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting chunk_cache for dataset %s successful", name_fqn);
      }
    }
    if (datasetProperties.access.filter_avail.use) {
    }
    if (datasetProperties.access.gzip.use) {
    }
    if (datasetProperties.access.szip.use) {
    }
    if (datasetProperties.access.virtual_view.use) {
    }
  }
  under = H5VLdataset_open(o->under_object, loc_params, o->under_vol_id, name,
                           dapl_id, dxpl_id, req);
  if (under) {
    dset = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    dset = NULL;

  return (void *)dset;
} /* end H5VL_intent_dataset_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_dataset_read
 *
 * Purpose:     Reads data elements from a dataset into a buffer.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_dataset_read(void *dset, hid_t mem_type_id,
                                       hid_t mem_space_id, hid_t file_space_id,
                                       hid_t plist_id, void *buf, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)dset;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATASET Read");
#endif

  ret_value = H5VLdataset_read(o->under_object, o->under_vol_id, mem_type_id,
                               mem_space_id, file_space_id, plist_id, buf, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_dataset_read() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_dataset_write
 *
 * Purpose:     Writes data elements from a buffer into a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_dataset_write(void *dset, hid_t mem_type_id,
                                        hid_t mem_space_id, hid_t file_space_id,
                                        hid_t plist_id, const void *buf,
                                        void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)dset;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATASET Write");
#endif

  ret_value =
      H5VLdataset_write(o->under_object, o->under_vol_id, mem_type_id,
                        mem_space_id, file_space_id, plist_id, buf, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_dataset_write() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_dataset_get
 *
 * Purpose:     Gets information about a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_dataset_get(void *dset, H5VL_dataset_get_args_t *args,
                                      hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)dset;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATASET Get");
#endif

  ret_value =
      H5VLdataset_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_dataset_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_dataset_specific
 *
 * Purpose:     Specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_dataset_specific(void *obj,
                                           H5VL_dataset_specific_args_t *args,
                                           hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  hid_t under_vol_id;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("H5Dspecific");
#endif

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  under_vol_id = o->under_vol_id;

  ret_value = H5VLdataset_specific(o->under_object, o->under_vol_id, args,
                                   dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, under_vol_id);

  return ret_value;
} /* end H5VL_intent_dataset_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_dataset_optional
 *
 * Purpose:     Perform a connector-specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_dataset_optional(void *obj,
                                           H5VL_optional_args_t *args,
                                           hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATASET Optional");
#endif

  ret_value = H5VLdataset_optional(o->under_object, o->under_vol_id, args,
                                   dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_dataset_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_dataset_close
 *
 * Purpose:     Closes a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1, dataset not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_dataset_close(void *dset, hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)dset;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATASET Close");
#endif

  ret_value = H5VLdataset_close(o->under_object, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying dataset was closed */
  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_dataset_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_datatype_commit
 *
 * Purpose:     Commits a datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_datatype_commit(void *obj,
                                         const H5VL_loc_params_t *loc_params,
                                         const char *name, hid_t type_id,
                                         hid_t lcpl_id, hid_t tcpl_id,
                                         hid_t tapl_id, hid_t dxpl_id,
                                         void **req) {
  H5VL_intent_t *dt;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATATYPE Commit");
#endif

  under =
      H5VLdatatype_commit(o->under_object, loc_params, o->under_vol_id, name,
                          type_id, lcpl_id, tcpl_id, tapl_id, dxpl_id, req);
  if (under) {
    dt = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    dt = NULL;

  return (void *)dt;
} /* end H5VL_intent_datatype_commit() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_datatype_open
 *
 * Purpose:     Opens a named datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_datatype_open(void *obj,
                                       const H5VL_loc_params_t *loc_params,
                                       const char *name, hid_t tapl_id,
                                       hid_t dxpl_id, void **req) {
  H5VL_intent_t *dt;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATATYPE Open");
#endif

  under = H5VLdatatype_open(o->under_object, loc_params, o->under_vol_id, name,
                            tapl_id, dxpl_id, req);
  if (under) {
    dt = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    dt = NULL;

  return (void *)dt;
} /* end H5VL_intent_datatype_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_datatype_get
 *
 * Purpose:     Get information about a datatype
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_datatype_get(void *dt, H5VL_datatype_get_args_t *args,
                                       hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)dt;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATATYPE Get");
#endif

  ret_value =
      H5VLdatatype_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_datatype_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_datatype_specific
 *
 * Purpose:     Specific operations for datatypes
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_datatype_specific(void *obj,
                                            H5VL_datatype_specific_args_t *args,
                                            hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  hid_t under_vol_id;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATATYPE Specific");
#endif

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  under_vol_id = o->under_vol_id;

  ret_value = H5VLdatatype_specific(o->under_object, o->under_vol_id, args,
                                    dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, under_vol_id);

  return ret_value;
} /* end H5VL_intent_datatype_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_datatype_optional
 *
 * Purpose:     Perform a connector-specific operation on a datatype
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_datatype_optional(void *obj,
                                            H5VL_optional_args_t *args,
                                            hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATATYPE Optional");
#endif

  ret_value = H5VLdatatype_optional(o->under_object, o->under_vol_id, args,
                                    dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_datatype_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_datatype_close
 *
 * Purpose:     Closes a datatype.
 *
 * Return:      Success:    0
 *              Failure:    -1, datatype not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_datatype_close(void *dt, hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)dt;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("DATATYPE Close");
#endif

  assert(o->under_object);

  ret_value =
      H5VLdatatype_close(o->under_object, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying datatype was closed */
  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_datatype_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_file_create
 *
 * Purpose:     Creates a container using this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_file_create(const char *name, unsigned flags,
                                     hid_t fcpl_id, hid_t fapl_id,
                                     hid_t dxpl_id, void **req) {
  H5VL_intent_info_t *info;
  H5VL_intent_t *file;
  hid_t under_fapl_id;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("FILE Create");
#endif

  /* Get copy of our VOL info from FAPL */
  H5Pget_vol_info(fapl_id, (void **)&info);

  /* Make sure we have info about the underlying VOL to be used */
  if (!info) return NULL;


  struct FileProperties fileProperties;
  bool is_present = get_file_properties(name, &fileProperties);
  if (is_present) {

#ifdef ENABLE_INTENT_LOGGING
    H5INTENT_LOGINFO("------- INTENT VOL FILE Found properties for file %s", name);
#endif
    if (fileProperties.creation.file_space.use){
      /**
       * Sets the file space page size for a file creation property list.
       *
       * Parameters
       *   [in]	plist_id	File creation property list identifier
       *   [in]	fsp_size	File space page size
       * Returns                Returns a non-negative value if successful;
       *                        otherwise returns a negative value.
       *
       * H5Pset_file_space_page_size() sets the file space page size fsp_size
       * used in paged aggregation and paged buffering. fsp_size has a minimum
       * size of 512. Setting a value less than 512 will return an error.
       * The library default size for the file space page size when not set
       * is 4096. The size set via this routine may not be changed for the life of the file.
       **/
      herr_t status = H5Pset_file_space_page_size(fcpl_id,
               fileProperties.creation.file_space.file_space_page_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting file_space_page_size for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting file_space_page_size for file %s successful", name);
      }
      status = H5Pset_file_space_strategy(fcpl_id,
                                                 fileProperties.creation.file_space.strategy,
                                                 fileProperties.creation.file_space.persist,
                                                 fileProperties.creation.file_space.threshold);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting file_space_strategy for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting file_space_strategy for file %s successful", name);
      }
    }
    if (fileProperties.creation.istore.use){
      herr_t status = H5Pset_istore_k(fcpl_id, fileProperties.creation.istore.ik);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting istore_k for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting istore_k for file %s successful", name);
      }
    }
    if (fileProperties.creation.sizes.use){
      herr_t status = H5Pset_sizes(fcpl_id,
                                   fileProperties.creation.sizes.sizeof_addr,
                                   fileProperties.creation.sizes.sizeof_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting set_sizes for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting set_sizes for file %s successful", name);
      }
    }
    if (fileProperties.access.cache.use){
      herr_t status = H5Pset_cache(fapl_id,
                                   fileProperties.access.cache.mdc_nelmts,
                                   fileProperties.access.cache.rdcc_nslots,
                                   fileProperties.access.cache.rdcc_nbytes,
                                   fileProperties.access.cache.rdcc_w0);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting set_cache for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting set_cache for file %s successful", name);
      }
    }
    if (fileProperties.access.close.use){
      H5F_close_degree_t degree = H5F_CLOSE_STRONG;
      if (strcmp(fileProperties.access.close.degree, "H5F_CLOSE_WEAK") == 0)
        degree = H5F_CLOSE_WEAK;
      herr_t status = H5Pset_fclose_degree(fapl_id, degree);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting fclose_degree for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting fclose_degree for file %s successful", name);
      }
    }
    if (fileProperties.access.core.use) {
      herr_t status = H5Pset_fapl_core(fapl_id,
                           fileProperties.access.core.increment,
                           fileProperties.access.core.backing_store);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting fapl_core for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting fapl_core for file %s successful", name);
      }
    }
    if (fileProperties.access.family.use) {
      herr_t status = H5Pset_fapl_family(fapl_id,
                                       fileProperties.access.family.memb_size,
                                       fileProperties.access.family.memb_fapl_id);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting fapl_family for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting fapl_family for file %s successful", name);
      }
    }
    if (fileProperties.access.file_image.use) {

    }
    if (fileProperties.access.fmpiio.use) {

    }
    if (fileProperties.access.log.use) {
      herr_t status = H5Pset_fapl_log(fapl_id,
                                         fileProperties.access.log.logfile,
                                      fileProperties.access.log.flags,
                                      fileProperties.access.log.buf_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting fapl_log for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting fapl_log for file %s successful", name);
      }
    }
    if (fileProperties.access.metadata.use) {
    }
    if (fileProperties.access.optimizations.use) {
      herr_t status = H5Pset_file_locking(fapl_id,
                                      fileProperties.access.optimizations.file_locking,
                                          0);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting file_locking for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting file_locking for file %s successful", name);
      }
      status = H5Pset_gc_references(fapl_id,
                                   fileProperties.access.optimizations.gc_ref);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting gc_references for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting gc_references for file %s successful", name);
      }
      status = H5Pset_sieve_buf_size(fapl_id,
                                    fileProperties.access.optimizations.sieve_buf_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting sieve_buf_size for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting sieve_buf_size for file %s successful", name);
      }
      status = H5Pset_small_data_block_size(fapl_id,
                                     fileProperties.access.optimizations.small_data_block_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting small_data_block_size for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting small_data_block_size for file %s successful", name);
      }
    }
    if (fileProperties.access.page_buffer.use) {
//      herr_t status = H5Pset_page_buffer_size(
//          fapl_id, fileProperties.access.page_buffer.buf_size, 100 - fileProperties.access.page_buffer.min_raw_per, fileProperties.access.page_buffer.min_raw_per);
//      if (status != 0) {
//        H5INTENT_LOGERROR("DATASET setting page_buffer_size for file %s failed",
//                          name);
//      } else {
//        H5INTENT_LOGINFO("DATASET setting page_buffer_size for file %s successful",
//                         name);
//      }
    }
    if (fileProperties.access.split.use) {
    }
    if (fileProperties.access.write_tracking.use) {
      herr_t status = H5Pset_core_write_tracking(
          fapl_id, 1, fileProperties.access.write_tracking.page_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting core_write_tracking for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO("DATASET setting core_write_tracking for file %s successful",
                         name);
      }
    }
  }


  /* Copy the FAPL */
  under_fapl_id = H5Pcopy(fapl_id);

  /* Set the VOL ID and info for the underlying FAPL */
  H5Pset_vol(under_fapl_id, info->under_vol_id, info->under_vol_info);

  /* Open the file with the underlying VOL connector */
  under = H5VLfile_create(name, flags, fcpl_id, under_fapl_id, dxpl_id, req);
  if (under) {
    file = H5VL_intent_new_obj(under, info->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, info->under_vol_id);
  } /* end if */
  else
    file = NULL;

  /* Close underlying FAPL */
  H5Pclose(under_fapl_id);

  /* Release copy of our VOL info */
  H5VL_intent_info_free(info);

  return (void *)file;
} /* end H5VL_intent_file_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_file_open
 *
 * Purpose:     Opens a container created with this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_file_open(const char *name, unsigned flags,
                                   hid_t fapl_id, hid_t dxpl_id, void **req) {
  H5VL_intent_info_t *info;
  H5VL_intent_t *file;
  hid_t under_fapl_id;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("FILE Open");
#endif
  struct FileProperties fileProperties;
  bool is_present = get_file_properties(name, &fileProperties);
  if (is_present) {

#ifdef ENABLE_INTENT_LOGGING
    H5INTENT_LOGINFO("------- INTENT VOL FILE Found properties for file %s", name);
#endif
    if (fileProperties.access.cache.use){
      herr_t status = H5Pset_cache(fapl_id,
                                   fileProperties.access.cache.mdc_nelmts,
                                   fileProperties.access.cache.rdcc_nslots,
                                   fileProperties.access.cache.rdcc_nbytes,
                                   fileProperties.access.cache.rdcc_w0);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting set_cache for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting set_cache for file %s successful", name);
      }
    }
    if (fileProperties.access.close.use){
      herr_t status = H5Pset_evict_on_close(fapl_id,
                                            fileProperties.access.close.evict);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting evict_on_close for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting evict_on_close for file %s successful", name);
      }
      H5F_close_degree_t degree = H5F_CLOSE_STRONG;
      if (strcmp(fileProperties.access.close.degree, "H5F_CLOSE_WEAK") == 0)
        degree = H5F_CLOSE_WEAK;
      status = H5Pset_fclose_degree(fapl_id, degree);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting fclose_degree for file %s failed", name);
      } else {
        H5INTENT_LOGINFO("DATASET setting fclose_degree for file %s successful", name);
      }
    }
    if (fileProperties.access.core.use) {
      herr_t status = H5Pset_fapl_core(fapl_id,
                                       fileProperties.access.core.increment,
                                       fileProperties.access.core.backing_store);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting fapl_core for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting fapl_core for file %s successful", name);
      }
    }
    if (fileProperties.access.family.use) {
      herr_t status = H5Pset_fapl_family(fapl_id,
                                         fileProperties.access.family.memb_size,
                                         fileProperties.access.family.memb_fapl_id);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting fapl_family for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting fapl_family for file %s successful", name);
      }
    }
    if (fileProperties.access.file_image.use) {

    }
    if (fileProperties.access.fmpiio.use) {

    }
    if (fileProperties.access.log.use) {
      herr_t status = H5Pset_fapl_log(fapl_id,
                                      fileProperties.access.log.logfile,
                                      fileProperties.access.log.flags,
                                      fileProperties.access.log.buf_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting fapl_log for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting fapl_log for file %s successful", name);
      }
    }
    if (fileProperties.access.metadata.use) {
    }
    if (fileProperties.access.optimizations.use) {
      herr_t status = H5Pset_file_locking(fapl_id,
                                          fileProperties.access.optimizations.file_locking,
                                          0);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting file_locking for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting file_locking for file %s successful", name);
      }
      status = H5Pset_gc_references(fapl_id,
                                    fileProperties.access.optimizations.gc_ref);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting gc_references for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting gc_references for file %s successful", name);
      }
      status = H5Pset_sieve_buf_size(fapl_id,
                                     fileProperties.access.optimizations.sieve_buf_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting sieve_buf_size for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting sieve_buf_size for file %s successful", name);
      }
      status = H5Pset_small_data_block_size(fapl_id,
                                            fileProperties.access.optimizations.small_data_block_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting small_data_block_size for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO(
            "DATASET setting small_data_block_size for file %s successful", name);
      }
    }
    if (fileProperties.access.page_buffer.use) {
      herr_t status = H5Pset_page_buffer_size(
          fapl_id, fileProperties.access.page_buffer.buf_size, fileProperties.access.page_buffer.min_meta_per, fileProperties.access.page_buffer.min_raw_per);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting page_buffer_size for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO("DATASET setting page_buffer_size for file %s successful",
                         name);
      }
    }
    if (fileProperties.access.split.use) {
    }
    if (fileProperties.access.write_tracking.use) {
      herr_t status = H5Pset_core_write_tracking(
          fapl_id, 1, fileProperties.access.write_tracking.page_size);
      if (status != 0) {
        H5INTENT_LOGERROR("DATASET setting core_write_tracking for file %s failed",
                          name);
      } else {
        H5INTENT_LOGINFO("DATASET setting core_write_tracking for file %s successful",
                         name);
      }
    }
  }
  /* Get copy of our VOL info from FAPL */
  H5Pget_vol_info(fapl_id, (void **)&info);

  /* Make sure we have info about the underlying VOL to be used */
  if (!info) return NULL;

  /* Copy the FAPL */
  under_fapl_id = H5Pcopy(fapl_id);

  /* Set the VOL ID and info for the underlying FAPL */
  H5Pset_vol(under_fapl_id, info->under_vol_id, info->under_vol_info);

  /* Open the file with the underlying VOL connector */
  under = H5VLfile_open(name, flags, under_fapl_id, dxpl_id, req);
  if (under) {
    file = H5VL_intent_new_obj(under, info->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, info->under_vol_id);
  } /* end if */
  else
    file = NULL;

  /* Close underlying FAPL */
  H5Pclose(under_fapl_id);

  /* Release copy of our VOL info */
  H5VL_intent_info_free(info);

  return (void *)file;
} /* end H5VL_intent_file_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_file_get
 *
 * Purpose:     Get info about a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_file_get(void *file, H5VL_file_get_args_t *args,
                                   hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)file;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("FILE Get");
#endif

  ret_value =
      H5VLfile_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_file_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_file_specific_reissue
 *
 * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
 *              file specific callback to the underlying VOL connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_file_specific_reissue(void *obj, hid_t connector_id,
                                                H5VL_file_specific_args_t *args,
                                                hid_t dxpl_id, void **req) {
  herr_t ret_value;
  ret_value = H5VLfile_specific(obj, connector_id, args, dxpl_id, req);
  return ret_value;
} /* end H5VL_intent_file_specific_reissue() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_file_specific
 *
 * Purpose:     Specific operation on file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_file_specific(void *file,
                                        H5VL_file_specific_args_t *args,
                                        hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)file;
  hid_t under_vol_id = -1;
  herr_t ret_value;
#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("File Specific");
#endif
  ret_value =
      H5VLfile_specific(o->under_object, o->under_vol_id, args, dxpl_id, req);
  return ret_value;
} /* end H5VL_intent_file_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_file_optional
 *
 * Purpose:     Perform a connector-specific operation on a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_file_optional(void *file, H5VL_optional_args_t *args,
                                        hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)file;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("File Optional");
#endif

  ret_value =
      H5VLfile_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_file_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_file_close
 *
 * Purpose:     Closes a file.
 *
 * Return:      Success:    0
 *              Failure:    -1, file not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_file_close(void *file, hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)file;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("FILE Close");
#endif

  ret_value = H5VLfile_close(o->under_object, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying file was closed */
  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_file_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_group_create
 *
 * Purpose:     Creates a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_group_create(void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      const char *name, hid_t lcpl_id,
                                      hid_t gcpl_id, hid_t gapl_id,
                                      hid_t dxpl_id, void **req) {
  H5VL_intent_t *group;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("GROUP Create");
#endif

  under = H5VLgroup_create(o->under_object, loc_params, o->under_vol_id, name,
                           lcpl_id, gcpl_id, gapl_id, dxpl_id, req);
  if (under) {
    group = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    group = NULL;

  return (void *)group;
} /* end H5VL_intent_group_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_group_open
 *
 * Purpose:     Opens a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_group_open(void *obj,
                                    const H5VL_loc_params_t *loc_params,
                                    const char *name, hid_t gapl_id,
                                    hid_t dxpl_id, void **req) {
  H5VL_intent_t *group;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("GROUP Open");
#endif

  under = H5VLgroup_open(o->under_object, loc_params, o->under_vol_id, name,
                         gapl_id, dxpl_id, req);
  if (under) {
    group = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    group = NULL;

  return (void *)group;
} /* end H5VL_intent_group_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_group_get
 *
 * Purpose:     Get info about a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_group_get(void *obj, H5VL_group_get_args_t *args,
                                    hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("GROUP Get");
#endif

  ret_value =
      H5VLgroup_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_group_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_group_specific
 *
 * Purpose:     Specific operation on a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_group_specific(void *obj,
                                         H5VL_group_specific_args_t *args,
                                         hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  hid_t under_vol_id;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("GROUP Specific");
#endif

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  under_vol_id = o->under_vol_id;

  ret_value =
      H5VLgroup_specific(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, under_vol_id);

  return ret_value;
} /* end H5VL_intent_group_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_group_optional
 *
 * Purpose:     Perform a connector-specific operation on a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_group_optional(void *obj, H5VL_optional_args_t *args,
                                         hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("GROUP Optional");
#endif

  ret_value =
      H5VLgroup_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_group_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_group_close
 *
 * Purpose:     Closes a group.
 *
 * Return:      Success:    0
 *              Failure:    -1, group not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_group_close(void *grp, hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)grp;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("H5Gclose");
#endif

  ret_value = H5VLgroup_close(o->under_object, o->under_vol_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  /* Release our wrapper, if underlying file was closed */
  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_group_close() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_link_create_reissue
 *
 * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
 *              link create callback to the underlying VOL connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_link_create_reissue(
    H5VL_link_create_args_t *args, void *obj,
    const H5VL_loc_params_t *loc_params, hid_t connector_id, hid_t lcpl_id,
    hid_t lapl_id, hid_t dxpl_id, void **req) {
  herr_t ret_value;
  ret_value = H5VLlink_create(args, obj, loc_params, connector_id, lcpl_id,
                              lapl_id, dxpl_id, req);
  return ret_value;
} /* end H5VL_intent_link_create_reissue() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_link_create
 *
 * Purpose:     Creates a hard / soft / UD / external link.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_link_create(H5VL_link_create_args_t *args, void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      hid_t lcpl_id, hid_t lapl_id,
                                      hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  hid_t under_vol_id = -1;
  herr_t ret_value;
  ret_value = H5VLlink_create(args, (o ? o->under_object : NULL), loc_params,
                              under_vol_id, lcpl_id, lapl_id, dxpl_id, req);
  return ret_value;
} /* end H5VL_intent_link_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_link_copy
 *
 * Purpose:     Renames an object within an HDF5 container and copies it to a
 *new group.  The original name SRC is unlinked from the group graph and then
 *inserted with the new name DST (which can specify a new path for the object)
 *as an atomic operation. The names are interpreted relative to SRC_LOC_ID and
 *              DST_LOC_ID, which are either file IDs or group ID.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_link_copy(void *src_obj,
                                    const H5VL_loc_params_t *loc_params1,
                                    void *dst_obj,
                                    const H5VL_loc_params_t *loc_params2,
                                    hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id,
                                    void **req) {
  H5VL_intent_t *o_src = (H5VL_intent_t *)src_obj;
  H5VL_intent_t *o_dst = (H5VL_intent_t *)dst_obj;
  hid_t under_vol_id = -1;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("LINK Copy");
#endif

  /* Retrieve the "under" VOL id */
  if (o_src)
    under_vol_id = o_src->under_vol_id;
  else if (o_dst)
    under_vol_id = o_dst->under_vol_id;
  assert(under_vol_id > 0);

  ret_value = H5VLlink_copy((o_src ? o_src->under_object : NULL), loc_params1,
                            (o_dst ? o_dst->under_object : NULL), loc_params2,
                            under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, under_vol_id);

  return ret_value;
} /* end H5VL_intent_link_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_link_move
 *
 * Purpose:     Moves a link within an HDF5 file to a new group.  The original
 *              name SRC is unlinked from the group graph
 *              and then inserted with the new name DST (which can specify a
 *              new path for the object) as an atomic operation. The names
 *              are interpreted relative to SRC_LOC_ID and
 *              DST_LOC_ID, which are either file IDs or group ID.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_link_move(void *src_obj,
                                    const H5VL_loc_params_t *loc_params1,
                                    void *dst_obj,
                                    const H5VL_loc_params_t *loc_params2,
                                    hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id,
                                    void **req) {
  H5VL_intent_t *o_src = (H5VL_intent_t *)src_obj;
  H5VL_intent_t *o_dst = (H5VL_intent_t *)dst_obj;
  hid_t under_vol_id = -1;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("LINK Move");
#endif

  /* Retrieve the "under" VOL id */
  if (o_src)
    under_vol_id = o_src->under_vol_id;
  else if (o_dst)
    under_vol_id = o_dst->under_vol_id;
  assert(under_vol_id > 0);

  ret_value = H5VLlink_move((o_src ? o_src->under_object : NULL), loc_params1,
                            (o_dst ? o_dst->under_object : NULL), loc_params2,
                            under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, under_vol_id);

  return ret_value;
} /* end H5VL_intent_link_move() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_link_get
 *
 * Purpose:     Get info about a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_link_get(void *obj,
                                   const H5VL_loc_params_t *loc_params,
                                   H5VL_link_get_args_t *args, hid_t dxpl_id,
                                   void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("LINK Get");
#endif

  ret_value = H5VLlink_get(o->under_object, loc_params, o->under_vol_id, args,
                           dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_link_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_link_specific
 *
 * Purpose:     Specific operation on a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_link_specific(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        H5VL_link_specific_args_t *args,
                                        hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("LINK Specific");
#endif

  ret_value = H5VLlink_specific(o->under_object, loc_params, o->under_vol_id,
                                args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_link_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_link_optional
 *
 * Purpose:     Perform a connector-specific operation on a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_link_optional(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        H5VL_optional_args_t *args,
                                        hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("LINK Optional");
#endif

  ret_value = H5VLlink_optional(o->under_object, loc_params, o->under_vol_id,
                                args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_link_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_object_open
 *
 * Purpose:     Opens an object inside a container.
 *
 * Return:      Success:    Pointer to object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *H5VL_intent_object_open(void *obj,
                                     const H5VL_loc_params_t *loc_params,
                                     H5I_type_t *opened_type, hid_t dxpl_id,
                                     void **req) {
  H5VL_intent_t *new_obj;
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  void *under;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("OBJECT Open");
#endif

  under = H5VLobject_open(o->under_object, loc_params, o->under_vol_id,
                          opened_type, dxpl_id, req);
  if (under) {
    new_obj = H5VL_intent_new_obj(under, o->under_vol_id);

    /* Check for async request */
    if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);
  } /* end if */
  else
    new_obj = NULL;

  return (void *)new_obj;
} /* end H5VL_intent_object_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_object_copy
 *
 * Purpose:     Copies an object inside a container.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_object_copy(void *src_obj,
                                      const H5VL_loc_params_t *src_loc_params,
                                      const char *src_name, void *dst_obj,
                                      const H5VL_loc_params_t *dst_loc_params,
                                      const char *dst_name, hid_t ocpypl_id,
                                      hid_t lcpl_id, hid_t dxpl_id,
                                      void **req) {
  H5VL_intent_t *o_src = (H5VL_intent_t *)src_obj;
  H5VL_intent_t *o_dst = (H5VL_intent_t *)dst_obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("OBJECT Copy");
#endif

  ret_value =
      H5VLobject_copy(o_src->under_object, src_loc_params, src_name,
                      o_dst->under_object, dst_loc_params, dst_name,
                      o_src->under_vol_id, ocpypl_id, lcpl_id, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o_src->under_vol_id);

  return ret_value;
} /* end H5VL_intent_object_copy() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_object_get
 *
 * Purpose:     Get info about an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_object_get(void *obj,
                                     const H5VL_loc_params_t *loc_params,
                                     H5VL_object_get_args_t *args,
                                     hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("OBJECT Get");
#endif

  ret_value = H5VLobject_get(o->under_object, loc_params, o->under_vol_id, args,
                             dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_object_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_object_specific
 *
 * Purpose:     Specific operation on an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_object_specific(void *obj,
                                          const H5VL_loc_params_t *loc_params,
                                          H5VL_object_specific_args_t *args,
                                          hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  hid_t under_vol_id;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("OBJECT Specific");
#endif

  // Save copy of underlying VOL connector ID and prov helper, in case of
  // refresh destroying the current object
  under_vol_id = o->under_vol_id;

  ret_value = H5VLobject_specific(o->under_object, loc_params, o->under_vol_id,
                                  args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, under_vol_id);

  return ret_value;
} /* end H5VL_intent_object_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_object_optional
 *
 * Purpose:     Perform a connector-specific operation for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_object_optional(void *obj,
                                          const H5VL_loc_params_t *loc_params,
                                          H5VL_optional_args_t *args,
                                          hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("OBJECT Optional");
#endif

  ret_value = H5VLobject_optional(o->under_object, loc_params, o->under_vol_id,
                                  args, dxpl_id, req);

  /* Check for async request */
  if (req && *req) *req = H5VL_intent_new_obj(*req, o->under_vol_id);

  return ret_value;
} /* end H5VL_intent_object_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_introspect_get_conn_clss
 *
 * Purpose:     Query the connector class.
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_intent_introspect_get_conn_cls(void *obj, H5VL_get_conn_lvl_t lvl,
                                           const H5VL_class_t **conn_cls) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("INTROSPECT GetConnCls");
#endif

  /* Check for querying this connector's class */
  if (H5VL_GET_CONN_LVL_CURR == lvl) {
    *conn_cls = &H5VL_intent_g;
    ret_value = 0;
  } /* end if */
  else
    ret_value = H5VLintrospect_get_conn_cls(o->under_object, o->under_vol_id,
                                            lvl, conn_cls);

  return ret_value;
} /* end H5VL_intent_introspect_get_conn_cls() */
/*-------------------------------------------------------------------------
 * Function:    H5VL_async_introspect_get_cap_flags
 *
 * Purpose:     Query the capability flags for this connector and any
 *              underlying connector(s).
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
#if H5_VERSION_GE(1, 13, 3)
H5VL_intent_introspect_get_cap_flags(const void *_info, uint64_t *cap_flags)
#else
H5VL_intent_introspect_get_cap_flags(const void *_info, unsigned *cap_flags)
#endif
{
  const H5VL_intent_info_t *info = (const H5VL_intent_info_t *)_info;
  herr_t ret_value;

#ifdef ENABLE_ASYNC_LOGGING
  H5INTENT_LOGINFO_SIMPLE("------- ASYNC VOL INTROSPECT GetCapFlags");
#endif

  /* Invoke the query on the underlying VOL connector */
  ret_value = H5VLintrospect_get_cap_flags(info->under_vol_info,
                                           info->under_vol_id, cap_flags);

  /* Bitwise OR our capability flags in */
  if (ret_value >= 0) *cap_flags |= H5VL_intent_g.cap_flags;

  return ret_value;
} /* end H5VL_async_introspect_get_cap_flags() */
/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_introspect_opt_query
 *
 * Purpose:     Query if an optional operation is supported by this connector
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_intent_introspect_opt_query(void *obj, H5VL_subclass_t cls,
                                        int opt_type, uint64_t *flags) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("INTROSPECT OptQuery");
#endif

  ret_value = H5VLintrospect_opt_query(o->under_object, o->under_vol_id, cls,
                                       opt_type, flags);

  return ret_value;
} /* end H5VL_intent_introspect_opt_query() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_request_wait
 *
 * Purpose:     Wait (with a timeout) for an async operation to complete
 *
 * Note:        Releases the request if the operation has completed and the
 *              connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_request_wait(void *obj, uint64_t timeout,
                                       H5VL_request_status_t *status) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("REQUEST Wait");
#endif

  ret_value =
      H5VLrequest_wait(o->under_object, o->under_vol_id, timeout, status);

  if (ret_value >= 0 && *status != H5ES_STATUS_IN_PROGRESS)
    H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_request_wait() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_request_notify
 *
 * Purpose:     Registers a user callback to be invoked when an asynchronous
 *              operation completes
 *
 * Note:        Releases the request, if connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_request_notify(void *obj, H5VL_request_notify_t cb,
                                         void *ctx) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("REQUEST Notify");
#endif

  ret_value = H5VLrequest_notify(o->under_object, o->under_vol_id, cb, ctx);

  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_request_notify() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_request_cancel
 *
 * Purpose:     Cancels an asynchronous operation
 *
 * Note:        Releases the request, if connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_request_cancel(void *obj,
                                         H5VL_request_status_t *status) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("REQUEST Cancel");
#endif

  ret_value = H5VLrequest_cancel(o->under_object, o->under_vol_id, status);

  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_request_cancel() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_request_specific_reissue
 *
 * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
 *              request specific callback to the underlying VOL connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_request_specific_reissue(
    void *obj, hid_t connector_id, H5VL_request_specific_args_t *args) {
  herr_t ret_value;
  ret_value = H5VLrequest_specific(obj, connector_id, args);

  return ret_value;
} /* end H5VL_intent_request_specific_reissue() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_request_specific
 *
 * Purpose:     Specific operation on a request
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_request_specific(void *obj,
                                           H5VL_request_specific_args_t *args) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("REQUEST Cancel");
#endif

  ret_value = H5VLrequest_specific(o->under_object, o->under_vol_id, args);

  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_request_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_request_optional
 *
 * Purpose:     Perform a connector-specific operation for a request
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_request_optional(void *obj,
                                           H5VL_optional_args_t *args) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("REQUEST Optional");
#endif

  ret_value = H5VLrequest_optional(o->under_object, o->under_vol_id, args);

  return ret_value;
} /* end H5VL_intent_request_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_request_free
 *
 * Purpose:     Releases a request, allowing the operation to complete without
 *              application tracking
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t H5VL_intent_request_free(void *obj) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("REQUEST Free");
#endif

  ret_value = H5VLrequest_free(o->under_object, o->under_vol_id);

  if (ret_value >= 0) H5VL_intent_free_obj(o);

  return ret_value;
} /* end H5VL_intent_request_free() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_blob_put
 *
 * Purpose:     Handles the blob 'put' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_intent_blob_put(void *obj, const void *buf, size_t size,
                            void *blob_id, void *ctx) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("BLOB Put");
#endif

  ret_value =
      H5VLblob_put(o->under_object, o->under_vol_id, buf, size, blob_id, ctx);

  return ret_value;
} /* end H5VL_intent_blob_put() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_blob_get
 *
 * Purpose:     Handles the blob 'get' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_intent_blob_get(void *obj, const void *blob_id, void *buf,
                            size_t size, void *ctx) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("BLOB Get");
#endif

  ret_value =
      H5VLblob_get(o->under_object, o->under_vol_id, blob_id, buf, size, ctx);

  return ret_value;
} /* end H5VL_intent_blob_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_blob_specific
 *
 * Purpose:     Handles the blob 'specific' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_intent_blob_specific(void *obj, void *blob_id,
                                 H5VL_blob_specific_args_t *args) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("BLOB Specific");
#endif

  ret_value =
      H5VLblob_specific(o->under_object, o->under_vol_id, blob_id, args);

  return ret_value;
} /* end H5VL_intent_blob_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_blob_optional
 *
 * Purpose:     Handles the blob 'optional' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_intent_blob_optional(void *obj, void *blob_id,
                                 H5VL_optional_args_t *args) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("BLOB Optional");
#endif

  ret_value =
      H5VLblob_optional(o->under_object, o->under_vol_id, blob_id, args);

  return ret_value;
} /* end H5VL_intent_blob_optional() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_token_cmp
 *
 * Purpose:     Compare two of the connector's object tokens, setting
 *              *cmp_value, following the same rules as strcmp().
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_token_cmp(void *obj, const H5O_token_t *token1,
                                    const H5O_token_t *token2, int *cmp_value) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("TOKEN Compare");
#endif

  /* Sanity checks */
  assert(obj);
  assert(token1);
  assert(token2);
  assert(cmp_value);

  ret_value = H5VLtoken_cmp(o->under_object, o->under_vol_id, token1, token2,
                            cmp_value);

  return ret_value;
} /* end H5VL_intent_token_cmp() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_token_to_str
 *
 * Purpose:     Serialize the connector's object token into a string.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_token_to_str(void *obj, H5I_type_t obj_type,
                                       const H5O_token_t *token,
                                       char **token_str) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("TOKEN To string");
#endif

  /* Sanity checks */
  assert(obj);
  assert(token);
  assert(token_str);

  ret_value = H5VLtoken_to_str(o->under_object, obj_type, o->under_vol_id,
                               token, token_str);

  return ret_value;
} /* end H5VL_intent_token_to_str() */

/*---------------------------------------------------------------------------
 * Function:    H5VL_intent_token_from_str
 *
 * Purpose:     Deserialize the connector's object token from a string.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t H5VL_intent_token_from_str(void *obj, H5I_type_t obj_type,
                                         const char *token_str,
                                         H5O_token_t *token) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("TOKEN From string");
#endif

  /* Sanity checks */
  assert(obj);
  assert(token);
  assert(token_str);

  ret_value = H5VLtoken_from_str(o->under_object, obj_type, o->under_vol_id,
                                 token_str, token);

  return ret_value;
} /* end H5VL_intent_token_from_str() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_intent_optional
 *
 * Purpose:     Handles the generic 'optional' callback
 *
 * Return:      SUCCEED / FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_intent_optional(void *obj, H5VL_optional_args_t *args,
                            hid_t dxpl_id, void **req) {
  H5VL_intent_t *o = (H5VL_intent_t *)obj;
  herr_t ret_value;

#ifdef ENABLE_INTENT_LOGGING
  H5INTENT_LOGINFO_SIMPLE("generic Optional");
#endif

  ret_value =
      H5VLoptional(o->under_object, o->under_vol_id, args, dxpl_id, req);

  return ret_value;
} /* end H5VL_intent_optional() */