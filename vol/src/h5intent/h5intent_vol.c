
/*-------------------------------------------------------------------------
*
* Created: h5intent_vol_private.c
* Oct 2022
* Hariharan Devarajan <hariharandev1@llnl.gov>
*
* Purpose:Defines the H5Intent VOL Plugin
*
*-------------------------------------------------------------------------
*/

#include <hdf5.h>
#include <libgen.h>
#include <h5intent/h5intent_vol_private.h>
#include <h5intent/h5intent_vol.h>
#include <cpp-logger/clogger.h>
#define H5INTENT_LOGGER_NAME "H5INTENT"
#define H5INTENT_LOGINFO(format, ...) \
  clog(CPP_LOGGER_INFO,H5INTENT_LOGGER_NAME, format, __VA_ARGS__);
#define H5INTENT_LOGWARN(format, ...) \
  clog(CPP_LOGGER_WARN,H5INTENT_LOGGER_NAME, format, __VA_ARGS__);
#define H5INTENT_LOGERROR(format, ...) \
  clog(CPP_LOGGER_ERROR,H5INTENT_LOGGER_NAME, format, __VA_ARGS__);
#define H5INTENT_LOGPRINT(format, ...) \
  clog(CPP_LOGGER_PRINT,H5INTENT_LOGGER_NAME, format, __VA_ARGS__);

hid_t H5INTENT_g = H5I_INVALID_HID;

/* H5Intent VOL other callbacks*/
static herr_t h5intent_init(hid_t vipl_id) {
    vipl_id = vipl_id;
    return 0;
}

static herr_t h5intent_term() {
    H5INTENT_g = H5I_INVALID_HID;
    return 0;
}

H5Intent_t *
h5intent_new_obj(void *under_obj, hid_t under_vol_id,H5I_type_t type) {
    H5Intent_t *new_obj;

    new_obj = (H5Intent_t *) calloc(1, sizeof(H5Intent_t));
    new_obj->under_object = under_obj;
    new_obj->under_vol_id = under_vol_id;
    if(type!=H5I_NTYPES)
        new_obj->object_id = H5VLwrap_register(new_obj,type);
    H5Iinc_ref(new_obj->under_vol_id);

    return (new_obj);
}

herr_t
h5intent_free_obj(H5Intent_t *obj) {
    H5Idec_ref(obj->under_vol_id);
    free(obj);
    return (0);
}

void *
h5intent_info_copy(const void *_info) {
    const H5Intent_info_t *info = (const H5Intent_info_t *) _info;
    H5Intent_info_t *new_info;

    /* Allocate new VOL info struct for the pass through connector */
    new_info = (H5Intent_info_t *) calloc(1, sizeof(H5Intent_info_t));

    /* Increment reference count on underlying VOL ID, and copy the VOL info */
    new_info->under_vol_id = info->under_vol_id;
    H5Iinc_ref(new_info->under_vol_id);
    if (info->under_vol_info)
        H5VLcopy_connector_info(new_info->under_vol_id, &(new_info->under_vol_info), info->under_vol_info);

    return (new_info);
}

herr_t
h5intent_info_cmp(int *cmp_value, const void *_info1, const void *_info2) {
    const H5Intent_info_t *info1 = (const H5Intent_info_t *) _info1;
    const H5Intent_info_t *info2 = (const H5Intent_info_t *) _info2;

    /* Sanity checks */
    assert(info1);
    assert(info2);

    /* Initialize comparison value */
    *cmp_value = 0;

    /* Compare under VOL connector classes */
    H5VLcmp_connector_cls(cmp_value, info1->under_vol_id, info2->under_vol_id);
    if (*cmp_value != 0)
        return (0);

    /* Compare under VOL connector info objects */
    H5VLcmp_connector_info(cmp_value, info1->under_vol_id, info1->under_vol_info, info2->under_vol_info);
    if (*cmp_value != 0)
        return (0);

    return (0);
}

herr_t h5intent_info_free(void *_info) {

    H5Intent_info_t *info = (H5Intent_info_t *) _info;

    /* Release underlying VOL ID and info */
    H5Idec_ref(info->under_vol_id);

    /* Free pass through info object itself */
    free(info);

    return (0);
}

herr_t
h5intent_info_to_str(const void *_info, char **str) {
    const H5Intent_info_t *info = (const H5Intent_info_t *) _info;
    H5VL_class_value_t under_value = (H5VL_class_value_t) -1;
    char *under_vol_string = NULL;
    size_t under_vol_str_len = 0;

    /* Get value and string for underlying VOL connector */
    H5VLget_value(info->under_vol_id, &under_value);
    H5VLconnector_info_to_str(info->under_vol_info, info->under_vol_id, &under_vol_string);

    /* Determine length of underlying VOL info string */
    if (under_vol_string)
        under_vol_str_len = strlen(under_vol_string);

    /* Allocate space for our info */
    *str = (char *) H5allocate_memory(32 + under_vol_str_len, (hbool_t) 0);
    assert(*str);

    /* Encode our info */
    snprintf(*str, 32 + under_vol_str_len, "under_vol=%u;under_info={%s}", (unsigned) under_value,
             (under_vol_string ? under_vol_string : ""));

    return (0);
}

herr_t
h5intent_str_to_info(const char *str, void **_info) {
    H5Intent_info_t *info;
    unsigned under_vol_value;
    const char *under_vol_info_start, *under_vol_info_end;
    hid_t under_vol_id;
    void *under_vol_info = NULL;

    /* Retrieve the underlying VOL connector value and info */
    sscanf(str, "under_vol=%u;", &under_vol_value);
    under_vol_id = H5VLregister_connector_by_value((H5VL_class_value_t) under_vol_value, H5P_DEFAULT);
    under_vol_info_start = strchr(str, '{');
    under_vol_info_end = strrchr(str, '}');
    assert(under_vol_info_end > under_vol_info_start);
    if (under_vol_info_end != (under_vol_info_start + 1)) {
        char *under_vol_info_str;

        under_vol_info_str = (char *) malloc((size_t) (under_vol_info_end - under_vol_info_start));
        memcpy(under_vol_info_str, under_vol_info_start + 1,
               (size_t) ((under_vol_info_end - under_vol_info_start) - 1));
        *(under_vol_info_str + (under_vol_info_end - under_vol_info_start)) = '\0';

        H5VLconnector_str_to_info(under_vol_info_str, under_vol_id, &under_vol_info);

        free(under_vol_info_str);
    } /* end else */

    /* Allocate new pass-through VOL connector info and set its fields */
    info = (H5Intent_info_t *) calloc(1, sizeof(H5Intent_info_t));
    info->under_vol_id = under_vol_id;
    info->under_vol_info = under_vol_info;

    /* Set return value */
    *_info = info;

    return (0);
}

void *
h5intent_get_object(const void *obj) {
    const H5Intent_t *o = (const H5Intent_t *) obj;
    return (H5VLget_object(o->under_object, o->under_vol_id));
}

herr_t
h5intent_get_wrap_ctx(const void *obj, void **wrap_ctx) {
    const H5Intent_t *o = (const H5Intent_t *) obj;
    H5Intent_wrap_ctx_t *new_wrap_ctx;
    /* Allocate new VOL object wrapping context for the pass through connector */
    new_wrap_ctx = (H5Intent_wrap_ctx_t *) calloc(1, sizeof(H5Intent_wrap_ctx_t));

    /* Increment reference count on underlying VOL ID, and copy the VOL info */
    new_wrap_ctx->under_vol_id = o->under_vol_id;
    H5Iinc_ref(new_wrap_ctx->under_vol_id);
    H5VLget_wrap_ctx(o->under_object, o->under_vol_id, &new_wrap_ctx->under_wrap_ctx);

    /* Set wrap context to return */
    *wrap_ctx = new_wrap_ctx;

    return (0);
}

void *
h5intent_wrap_object(void *obj, H5I_type_t obj_type, void *_wrap_ctx) {
    H5Intent_wrap_ctx_t *wrap_ctx = (H5Intent_wrap_ctx_t *) _wrap_ctx;
    H5Intent_t *new_obj;
    void *under;

    /* Wrap the object with the underlying VOL */
    under = H5VLwrap_object(obj, obj_type, wrap_ctx->under_vol_id, wrap_ctx->under_wrap_ctx);
    if (under)
        new_obj = h5intent_new_obj(under, wrap_ctx->under_vol_id,H5I_NTYPES);
    else
        new_obj = NULL;

    return (new_obj);
} /* end h5intent_wrap_object() */

herr_t
h5intent_free_wrap_ctx(void *_wrap_ctx) {
    H5Intent_wrap_ctx_t *wrap_ctx = (H5Intent_wrap_ctx_t *) _wrap_ctx;

    /* Release underlying VOL ID and wrap context */
    if (wrap_ctx->under_wrap_ctx)
        H5VLfree_wrap_ctx(wrap_ctx->under_wrap_ctx, wrap_ctx->under_vol_id);
    H5Idec_ref(wrap_ctx->under_vol_id);

    /* Free pass through wrap context object itself */
    free(wrap_ctx);

    return (0);
}

hid_t h5intent_register(void) {
    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Singleton register the pass-through VOL connector ID */
    if (H5I_VOL != H5Iget_type(H5INTENT_g))
        H5INTENT_g = H5VLregister_connector(&H5Intent_g, H5P_DEFAULT);

    return (H5INTENT_g);
}


/* H5Intent VOL Dataset callbacks */
void *h5intent_dataset_create(void *obj, const H5VL_loc_params_t *loc_params,
                            const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id,
                            hid_t dapl_id, hid_t dxpl_id, void **req) {
    H5INTENT_LOGPRINT("Creating dataset %s", name);
    H5Intent_t *dset;
    H5Intent_t *o = (H5Intent_t *) obj;
    void *under;

#ifdef ENABLE_LOGGING
    printf("------- PASS THROUGH VOL DATASET Create\n");
#endif

    under = H5VLdataset_create(o->under_object, loc_params, o->under_vol_id, name, lcpl_id, type_id, space_id, dcpl_id,
                               dapl_id, dxpl_id, req);
    return ((void *) dset);
}

static void *h5intent_dataset_open(void *obj, const H5VL_loc_params_t *loc_params,
                                 const char *name, hid_t dapl_id, hid_t dxpl_id, void **req) {
    H5INTENT_LOGPRINT("Opening file %s", name);
    H5Intent_t *dset;
    H5Intent_t *o = (H5Intent_t *) obj;
    void *under;
    under = H5VLdataset_open(o->under_object, loc_params, o->under_vol_id, name, dapl_id, dxpl_id, req);
    return ((void *) dset);
}
void dataset_get_wrapper(void *dset, hid_t driver_id, H5VL_dataset_get_args_t *args, hid_t dxpl_id, void **req)
{
    H5VLdataset_get(dset, driver_id, args, dxpl_id, req);
}

static herr_t
h5intent_dataset_read(void *obj, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, void *buf,
                    void **req) {
    H5Intent_t *o = (H5Intent_t *) obj;
    herr_t ret = H5VLdataset_read(o->under_object, o->under_vol_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);
    return ret;
}

static herr_t
h5intent_dataset_write(void *obj, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id,
                     const void *buf, void **req) {
    H5Intent_t *o = (H5Intent_t *) obj;
    herr_t ret = H5VLdataset_write(o->under_object, o->under_vol_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);
    return ret;
}

static herr_t
h5intent_dataset_get(void *dset, H5VL_dataset_get_args_t *args, hid_t dxpl_id, void **req) {
    H5Intent_t *o = (H5Intent_t *) dset;
    herr_t ret_value;
    ret_value = H5VLdataset_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return (ret_value);
}

herr_t
h5intent_dataset_specific(void *obj, H5VL_dataset_specific_args_t *args,
                        hid_t dxpl_id, void **req) {
    H5Intent_t *o = (H5Intent_t *) obj;
    herr_t ret_value;
    ret_value = H5VLdataset_specific(o->under_object, o->under_vol_id, args, dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return (ret_value);
}

herr_t
h5intent_dataset_optional(void *obj, H5VL_optional_args_t  *args,hid_t dxpl_id, void **req) {
    H5Intent_t *o = (H5Intent_t *) obj;
    herr_t ret_value;
    ret_value = H5VLdataset_optional(o->under_object, o->under_vol_id, args,dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return (ret_value);
}

static herr_t h5intent_dataset_close(void *obj, hid_t dxpl_id, void **req) {
    H5Intent_t *o = (H5Intent_t *) obj;
    herr_t ret_value;
    ret_value = H5VLdataset_close(o->under_object, o->under_vol_id, dxpl_id, req);
    return ret_value;
}

/* H5intent VOL File callbacks */
static void *
h5intent_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req) {
    H5Intent_info_t *info;
    H5Intent_t *file;
    hid_t under_fapl_id;
    void *under;
    /* Get copy of our VOL info from FAPL */
    H5Pget_vol_info(fapl_id, (void **) &info);
    if(!info){
        info=malloc(sizeof(H5Intent_info_t));
        info->under_vol_id=H5VLget_connector_id(H5VL_NATIVE);
    }

    /* Copy the FAPL */
    under_fapl_id = H5Pcopy(fapl_id);

    /* Set the VOL ID and info for the underlying FAPL */
    H5Pset_vol(under_fapl_id, info->under_vol_id, NULL);

    /* Open the file with the underlying VOL connector */
    under = H5VLfile_create(name, flags, fcpl_id, under_fapl_id, dxpl_id, req);
    if (under) {
        file = h5intent_new_obj(under, info->under_vol_id,H5I_NTYPES);

        /* Check for async request */
        if (req && *req)
            *req = h5intent_new_obj(*req, info->under_vol_id,H5I_NTYPES);
        file->file_name = strdup(name);
    } /* end if */
    else
        file = NULL;



    /* Close underlying FAPL */
    H5Pclose(under_fapl_id);

    /* Release copy of our VOL info */
    h5intent_info_free(info);
    H5INTENT_LOGPRINT("Creating file %s", name);
    return ((void *) file);
}

static void *h5intent_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req) {
    H5Intent_info_t *info;
    H5Intent_t *file;
    hid_t under_fapl_id;
    void *under;

    /* Get copy of our VOL info from FAPL */
    H5Pget_vol_info(fapl_id, (void **) &info);
    if(!info){
        info=malloc(sizeof(H5Intent_info_t));
        info->under_vol_id=H5VLget_connector_id(H5VL_NATIVE);
    }

    /* Copy the FAPL */
    under_fapl_id = H5Pcopy(fapl_id);

    /* Set the VOL ID and info for the underlying FAPL */
    H5Pset_vol(under_fapl_id, info->under_vol_id, NULL);

    /* Open the file with the underlying VOL connector */
    under = H5VLfile_open(name, flags, under_fapl_id, dxpl_id, req);
    if (under) {
        file = h5intent_new_obj(under, info->under_vol_id,H5I_FILE);
        /* Check for async request */
        if (req && *req)
            *req = h5intent_new_obj(*req, info->under_vol_id,H5I_FILE);

        file->file_name = strdup(name);
    } /* end if */
    else {
        file = NULL;
    }

    /* Close underlying FAPL */
    H5Pclose(under_fapl_id);

    /* Release copy of our VOL info */
    h5intent_info_free(info);
    H5INTENT_LOGPRINT("Opening file %s", name);

    return ((void *) file);
}

herr_t
h5intent_file_get(void *file, H5VL_file_get_args_t *args, hid_t dxpl_id,
                void **req) {
    H5Intent_t *o = (H5Intent_t *) file;
    herr_t ret_value;
    ret_value = H5VLfile_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_FILE);

    return (ret_value);
}

herr_t h5intent_file_specific_reissue(void *obj, hid_t connector_id,
                                      H5VL_file_specific_args_t *args, hid_t dxpl_id, void **req) {
    va_list arguments;
    herr_t ret_value;
    ret_value = H5VLfile_specific(obj, connector_id, args, dxpl_id, req);
    return (ret_value);
}

herr_t
h5intent_file_specific(void *file, H5VL_file_specific_args_t *args,
                     hid_t dxpl_id, void **req) {
    H5Intent_t *o = (H5Intent_t *) file;
    hid_t under_vol_id = -1;
    herr_t ret_value = H5VLfile_specific(o->under_object, o->under_vol_id, args, dxpl_id, req);
    return ret_value;
}

herr_t h5intent_file_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id,
                              void **req) {
    H5Intent_t *o = (H5Intent_t *) obj;
    herr_t ret_value;

    ret_value = H5VLfile_optional(o->under_object, o->under_vol_id, args,dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return (ret_value);
}

herr_t
h5intent_file_close(void *file, hid_t dxpl_id, void **req) {
    H5Intent_t *o = (H5Intent_t *) file;
    herr_t ret_value;
    ret_value = H5VLfile_close(o->under_object, o->under_vol_id, dxpl_id, req);

    /* Check for async request */
    if (req && *req) *req = h5intent_new_obj(*req, o->under_vol_id,H5I_FILE);

    /* Release our wrapper, if underlying file was closed */
    if (ret_value >= 0) {
        free(o->file_name);
        o->file_name = NULL;
        h5intent_free_obj(o);
    }

    return (ret_value);
}


H5PL_type_t H5PLget_plugin_type(void) {
    return H5PL_TYPE_VOL;
}


const void *H5PLget_plugin_info(void) {
    return &H5Intent_g;
}

void *
h5intent_group_create(void *obj, const H5VL_loc_params_t *loc_params,
                    const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req) {
    H5Intent_t *group;
    H5Intent_t *o = (H5Intent_t *) obj;
    void *under;

#ifdef ENABLE_LOGGING
    printf("------- PASS THROUGH VOL GROUP Create\n");
#endif

    under = H5VLgroup_create(o->under_object, loc_params, o->under_vol_id, name, lcpl_id, gcpl_id, gapl_id, dxpl_id,
                             req);
    if (under) {
        group = h5intent_new_obj(under, o->under_vol_id,H5I_GROUP);

        /* Check for async request */
        if (req && *req)
            *req = h5intent_new_obj(*req, o->under_vol_id,H5I_GROUP);
        group->file_name = o->file_name;
        group->group_name = strdup(name);
    } /* end if */
    else
        group = NULL;

    return ((void *) group);
} /* end h5intent_group_create() */

void *
h5intent_group_open(void *obj, const H5VL_loc_params_t *loc_params,
                  const char *name, hid_t gapl_id, hid_t dxpl_id, void **req) {
    H5Intent_t *group;
    H5Intent_t *o = (H5Intent_t *) obj;
    void *under;

    under = H5VLgroup_open(o->under_object, loc_params, o->under_vol_id, name, gapl_id, dxpl_id, req);
    if (under) {
        group = h5intent_new_obj(under, o->under_vol_id,H5I_GROUP);

        /* Check for async request */
        if (req && *req)
            *req = h5intent_new_obj(*req, o->under_vol_id,H5I_GROUP);


        group->file_name = o->file_name;
        group->group_name = strdup(name);


    } /* end if */
    else
        group = NULL;

    return ((void *) group);
} /* end h5intent_group_open() */

herr_t
h5intent_group_get(void *obj, H5VL_group_get_args_t *args, hid_t dxpl_id,
                 void **req) {
    H5Intent_t *o = (H5Intent_t *) (obj);
    herr_t ret_value;

    ret_value = H5VLgroup_get(o->under_object, o->under_vol_id, args, dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return (ret_value);
} /* end h5intent_group_get() */

/*-------------------------------------------------------------------------
 * Function:    h5intent_group_specific
 *
 * Purpose: Specific operation on a group
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t
h5intent_group_specific(void *obj, H5VL_group_specific_args_t *args,
                      hid_t dxpl_id, void **req) {
    H5Intent_t *o = (H5Intent_t *) obj;
    herr_t ret_value;

#ifdef ENABLE_LOGGING
    printf("------- PASS THROUGH VOL GROUP Specific\n");
#endif

    ret_value = H5VLgroup_specific(o->under_object, o->under_vol_id, args, dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return (ret_value);
} /* end h5intent_group_specific() */

/*-------------------------------------------------------------------------
 * Function:    h5intent_group_optional
 *
 * Purpose:     Perform a connector-specific operation on a group
 *
 * Return:  Success:    0
 *      Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t
h5intent_group_optional(void *obj, H5VL_optional_args_t *args, hid_t dxpl_id,
                        void **req) {
    H5Intent_t *o = (H5Intent_t *) obj;
    herr_t ret_value;

#ifdef ENABLE_LOGGING
    printf("------- PASS THROUGH VOL GROUP Optional\n");
#endif

    ret_value = H5VLgroup_optional(o->under_object, o->under_vol_id, args, dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return (ret_value);
} /* end h5intent_group_optional() */

/*-------------------------------------------------------------------------
 * Function:    h5intent_group_close
 *
 * Purpose:     Closes a group.
 *
 * Return:  Success:    0
 *      Failure:    -1, group not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t
h5intent_group_close(void *grp, hid_t dxpl_id, void **req) {
    H5Intent_t *o = (H5Intent_t *) grp;
    herr_t ret_value;

    if(o->object_id==0)
        return 0;

    ret_value = H5VLgroup_close(o->under_object, o->under_vol_id, dxpl_id, req);

    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    free(o->group_name);
    o->group_name = NULL;
    return (ret_value);
}

void *h5intent_attribute_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t type_id,
                              hid_t space_id, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req) {
    H5Intent_t *attr;
    H5Intent_t *o = (H5Intent_t *) obj;
    void *under;

    under = H5VLattr_create(o->under_object, loc_params, o->under_vol_id, name, type_id, space_id, acpl_id, aapl_id, dxpl_id, req);
    if (under) {
        attr = h5intent_new_obj(under, o->under_vol_id,H5I_ATTR);

        /* Check for async request */
        if (req && *req)
            *req = h5intent_new_obj(*req, o->under_vol_id,H5I_ATTR);
    } /* end if */
    else
        attr = NULL;

    return ((void *) attr);
}

void *
h5intent_attribute_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t aapl_id, hid_t dxpl_id,
                      void **req) {
    H5Intent_t *attr;
    H5Intent_t *o = (H5Intent_t *) obj;
    void *under;

    under = H5VLattr_open(o->under_object, loc_params, o->under_vol_id, name, aapl_id, dxpl_id, req);
    if (under) {
        attr = h5intent_new_obj(under, o->under_vol_id,H5I_ATTR);

        /* Check for async request */
        if (req && *req)
            *req = h5intent_new_obj(*req, o->under_vol_id,H5I_ATTR);
    } /* end if */
    else
        attr = NULL;

    return ((void *) attr);
}

herr_t h5intent_attribute_read(void *_attr, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req) {
    H5Intent_t *attr;
    H5Intent_t *o = (H5Intent_t *) _attr;
    herr_t ret_value;
    ret_value = H5VLattr_read(o->under_object,o->under_vol_id, mem_type_id, buf, dxpl_id, req);
    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return ret_value;
}

herr_t h5intent_attribute_write(void *_attr, hid_t mem_type_id, const void *buf, hid_t dxpl_id, void **req) {
    H5Intent_t *attr;
    H5Intent_t *o = (H5Intent_t *) _attr;
    herr_t ret_value;
    ret_value = H5VLattr_write(o->under_object,o->under_vol_id, mem_type_id, buf, dxpl_id, req);
    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return ret_value;
}

herr_t h5intent_attribute_get(void *_attr, H5VL_attr_get_args_t *args, hid_t dxpl_id, void **req) {
    H5Intent_t *attr;
    H5Intent_t *o = (H5Intent_t *) _attr;
    herr_t ret_value;
    ret_value = H5VLattr_get(o->under_object,o->under_vol_id, args, dxpl_id, req);
    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return ret_value;
}

herr_t h5intent_attribute_specific(void *_attr, const H5VL_loc_params_t *loc_params, H5VL_attr_specific_args_t *args,
                                 hid_t dxpl_id, void **req) {
    H5Intent_t *attr;
    H5Intent_t *o = (H5Intent_t *) _attr;
    herr_t ret_value;
    ret_value = H5VLattr_specific(o->under_object,loc_params, o->under_vol_id,  args,dxpl_id, req);
    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return ret_value;
}

herr_t h5intent_attribute_close(void *_attr, hid_t dxpl_id, void **req) {
    H5Intent_t *attr;
    H5Intent_t *o = (H5Intent_t *) _attr;
    if(o->object_id==0)
        return 0;
    herr_t ret_value;
    ret_value = H5VLattr_close(o->under_object,o->under_vol_id, dxpl_id, req);
    /* Check for async request */
    if (req && *req)
        *req = h5intent_new_obj(*req, o->under_vol_id,H5I_NTYPES);

    return ret_value;
}
/* end h5intent_group_close() */